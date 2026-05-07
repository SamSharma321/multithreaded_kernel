#include "fat16.h"
#include "disk/disk.h"
#include "status.h"
#include "string/string.h"
#include "stdint.h"
#include "disk/disk.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "disk/streamer.h"

#define SAMOS_FAT16_SIGNATURE       0x29
#define SAMOS_FAT16_FAT_ENTRY_SIZE  0x02
#define SAMOS_FAT16_BAD_SECTOR      0xFF7
#define SAMOS_FAT16_UNUSED          0x00

typedef unsigned int fat_item_type;
#define FAT_ITEM_TYPE_DIRECTORY 0       // Internal code use only
#define FAT_ITEM_TYPE_FILE      1

#define DIRECTORY_FREE      0xE5
#define DIRECTORY_EMPTY     0x00

//Create FAT directory entry attributes bit masks
#define FAT_FILE_READ_ONLY      0x01
#define FAT_FILE_HIDEEN         0x02
#define FAT_FILE_SYSTEM         0x04
#define FAT_FILE_VOLUME_LABEL   0x08
#define FAT_FILE_SUBDIRECTORY   0x10
#define FAT_FILE_ARCHIVE        0x20
#define FAT_FILE_DEVICE         0x40
#define FAT_FILE_RESERVED       0x80

// Refer the boot.asm for the replication of register configuration for fat16
struct fat_header_extended {
    uint8_t drive_no;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_str[11];
    uint8_t system_id_str[8];
} __attribute__((packed));  // Ensure the compiler does not rearrange the structure - since it is continguous memory

struct fat_header {
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));

struct fat_h {
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header; // This means the extended header is optional - future proofing
    } shared;
};

struct fat_directory_item {
    // Fillling of the directory into the structure to access them
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;  // Contains the above defined bit masks (MACROS)
    uint8_t reserved;
    uint8_t creation_time_tens_of_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

struct fat_directory {
    struct fat_directory_item* item;    // Points to first item in the directory, item[1] will be first item in directory
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item {
    union {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    } shared;
    fat_item_type type;
};

struct fat_item_decscriptor {
    struct fat_item* item;
    uint32_t pos;           // Position to be seeked in the file
};

struct fat_private {        // Internal use only
    struct fat_h header;
    struct fat_directory root_directory;
    // Used to stream data from the cluster
    struct disk_stream* cluster_read_stream;
    // Used to stream the file allocation table
    struct disk_stream* fat_read_stream;
    // Used in situations where we stream the directory
    struct disk_stream* directory_stream;
};

/* Function Prototypes */
int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct part_path* path, FILE_MODE mode);

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
};

struct filesystem* fat16_init() {
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

static void fat16_init_private(struct disk* disk, struct fat_private* private) {
    // Zero it out
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
}

static int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector) {
    int res = 0;
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* private = disk->fs_private;
    int i = 0;
    int directory_start_pos = directory_start_sector + disk->sector_size;
    struct disk_stream* stream = private->directory_stream;
    if (SAMOS_ALL_OK != diskstreamer_seek(stream, directory_start_pos)) {
        res = -EIO;
        goto out;
    }

    while(1) {
        if (diskstreamer_read(stream, &item, sizeof(item)) != SAMOS_ALL_OK) {
            res = -EIO;
            goto out;
        }
        if (item.filename[0] == DIRECTORY_EMPTY) { // blank record
            break;    
        }
        // If item is unused
        if (item.filename[0] == DIRECTORY_FREE) { // 0xE5 indicates if the directory entry is available (free)
            continue;
        }
        i++;
    }
    res = i; // response
out:
    return res;
}

// Returns the absolute number of bytes for the sector selected
static int fat16_sector_to_absolute(struct disk* disk, int sector) {
    return sector * disk->sector_size;
}

int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory) {
    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + \
                                    primary_header->reserved_sectors;
    int root_directory_entries = fat_private->header.primary_header.root_dir_entries;
    // This contains the fat directory items
    int root_dir_size = root_directory_entries * sizeof(struct fat_directory_item);
    int total_sectors = root_dir_size / disk->sector_size;
    if (root_dir_size % disk->sector_size) {
        total_sectors += 1; // if not a perfect multiple
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    struct fat_directory_item* dir = kzalloc(root_dir_size);
    if (!dir) {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != SAMOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    if (diskstreamer_read(stream, dir, root_dir_size) != SAMOS_ALL_OK) {
        res = -EIO;
        goto out;
    }
    // Update all calculated fields
    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;
}

// This function should return zero for proper functioning
int fat16_resolve(struct disk* disk) {
    int res = 0;
    struct fat_private* private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, private);

    disk->fs_private = private;
    disk->filesystem = &fat16_fs;   // Bind the filesystem to the disk

    struct disk_stream* stream = diskstreamer_new(disk->id);
    if(!stream) {
        res = -ENOMEM;
        goto out;
    }

    if (diskstreamer_read(stream, &private->header, sizeof(private->header)) != SAMOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    // Check if the header has the valid fat16 filesystem signature
    if (private->header.shared.extended_header.signature != 0x29) {
        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, private, &private->root_directory) != SAMOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

out:
    if (stream) {
        diskstreamer_close(stream);
    }
    if (res < 0) {
        kfree(private);
        disk->fs_private = 0;       // Assign null
    }
    return res;
}

void* fat16_open(struct disk* disk, struct part_path* path, FILE_MODE mode) {
    return (void*)0;
}
