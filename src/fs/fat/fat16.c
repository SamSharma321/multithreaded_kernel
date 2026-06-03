#include "fat16.h"
#include "disk/disk.h"
#include "status.h"
#include "string/string.h"
#include "stdint.h"
#include "disk/disk.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "disk/streamer.h"
#include "config.h"
#include "kernel.h"
#include "fs/pparser.h"

#define SAMOS_FAT16_SIGNATURE       0x29
#define SAMOS_FAT16_FAT_ENTRY_SIZE  0x02
#define SAMOS_FAT16_BAD_SECTOR      0xFFF7
#define SAMOS_FAT16_EOC             0xFFF8
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
#define FAT_FILE_LONG_NAME      0x0F

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

struct fat_file_descriptor {
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
int fat16_read(struct disk* disk, void* desc, uint32_t size,  uint32_t nmemb, char* out_ptr);
int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);
int fat16_stat(struct disk* disk, void* private, struct file_stat* stat);
int fat16_close(void* private);
void fat16_fat_item_free(struct fat_item* item);

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close
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

static void fat16_free_root_directory(struct fat_private* private) {
    if (private && private->root_directory.item) {
        kfree(private->root_directory.item);
        private->root_directory.item = 0;
    }
}

static void fat16_free_private(struct fat_private* private) {
    if (!private)
        return;

    fat16_free_root_directory(private);
    if (private->cluster_read_stream)
        diskstreamer_close(private->cluster_read_stream);
    if (private->fat_read_stream)
        diskstreamer_close(private->fat_read_stream);
    if (private->directory_stream)
        diskstreamer_close(private->directory_stream);

    kfree(private);
}

static int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector, int max_items) {
    int res = 0;
    struct fat_directory_item item;

    if (max_items <= 0) {
        goto out;
    }

    struct fat_private* private = disk->fs_private;
    int i = 0;
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream* stream = private->directory_stream;
    if (SAMOS_ALL_OK != diskstreamer_seek(stream, directory_start_pos)) {
        res = -EIO;
        goto out;
    }

    while(i < max_items) {
        if (diskstreamer_read(stream, &item, sizeof(item)) != SAMOS_ALL_OK) {
            res = -EIO;
            goto out;
        }
        if (item.filename[0] == DIRECTORY_EMPTY) { // blank record
            break;    
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
    struct fat_directory_item* dir = 0;
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

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos, root_directory_entries);
    if (total_items < 0) {
        res = total_items;
        goto out;
    }

    dir = kzalloc(root_dir_size);
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
    directory->ending_sector_pos = root_dir_sector_pos + total_sectors;

out:
    if (res < 0 && dir) {
        kfree(dir);
    }
    return res;
}

// This function should return zero for proper functioning
int fat16_resolve(struct disk* disk) {
    int res = 0;
    struct disk_stream* stream = 0;
    struct fat_private* private = kzalloc(sizeof(struct fat_private));
    if (!private) {
        res = -ENOMEM;
        goto out;
    }
    fat16_init_private(disk, private);
    if (!private->cluster_read_stream || !private->fat_read_stream || !private->directory_stream) {
        res = -ENOMEM;
        goto out;
    }

    disk->fs_private = private;
    disk->filesystem = &fat16_fs;   // Bind the filesystem to the disk

    stream = diskstreamer_new(disk->id);
    if(!stream) {
        res = -ENOMEM;
        goto out;
    }

    if (diskstreamer_read(stream, &private->header, sizeof(private->header)) != SAMOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    // Check if the header has the valid fat16 filesystem signature
    if (private->header.shared.extended_header.signature != SAMOS_FAT16_SIGNATURE) {
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
        fat16_free_private(private);
        disk->fs_private = 0;       // Assign null
        disk->filesystem = 0;
    }
    return res;
}

/* Copy a fixed-width FAT name field, trimming null and space padding. */
static void fat16_to_proper_string(char** out, int* remaining, const uint8_t* in, int in_len) {
    while (in_len-- > 0 && *remaining > 1 && *in != 0x00 && *in != 0x20) {
        **out = *in;
        *out += 1;
        in += 1;
        *remaining -= 1;
    }

    if (*remaining > 0) {
        **out = 0x00;
    }
}

/* */
void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len) {
    if (max_len <= 0)
        return;

    memset(out, 0x00, max_len);
    char* out_temp = out;
    int remaining = max_len;
    fat16_to_proper_string(&out_temp, &remaining, item->filename, sizeof(item->filename));
    if (remaining > 1 && item->ext[0] != 0x00 && item->ext[0] != 0x20) {
        *out_temp++ = '.';  // Add . before putting the extension. Eg: .exe
        remaining--;
        fat16_to_proper_string(&out_temp, &remaining, item->ext, sizeof(item->ext));
    }
}

struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size) {
    struct fat_directory_item* item_copy = 0;
    if (size < sizeof(struct fat_directory_item)) {
        return 0;
    }
    item_copy = kzalloc(size);
    if(!item_copy)
        return 0;
    memcpy(item_copy, item, size);
    return item_copy;
}

static int fat16_get_first_cluster(struct fat_directory_item* item) {
    return (item->high_16_bits_first_cluster << 16) | item->low_16_bits_first_cluster;
}

static int fat16_cluster_to_sector(struct fat_private* private, int cluster) {
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private* private) {
    return private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry(struct disk* disk, int cluster) {
    int res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_stream* stream = private->fat_read_stream;
    if (!stream)
        goto out;

    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    res = diskstreamer_seek(stream, fat_table_position + (cluster * SAMOS_FAT16_FAT_ENTRY_SIZE));
    if (res < 0) goto out;

    uint16_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result));
    if (res < 0) goto out;

    res = result;
out:
    return res;
}

static int fat16_stream_read(struct disk* disk, struct disk_stream* stream, int pos, int total, void* out) {
    int res = 0;
    int total_read = 0;

    while (total_read < total) {
        int current_pos = pos + total_read;
        int sector_offset = current_pos % disk->sector_size;
        int bytes_left_in_sector = disk->sector_size - sector_offset;
        int bytes_left = total - total_read;
        int bytes_to_read = bytes_left > bytes_left_in_sector ? bytes_left_in_sector : bytes_left;

        res = diskstreamer_seek(stream, current_pos);
        if (res != SAMOS_ALL_OK)
            goto out;

        res = diskstreamer_read(stream, (char*)out + total_read, bytes_to_read);
        if (res != SAMOS_ALL_OK)
            goto out;

        total_read += bytes_to_read;
    }

out:
    return res;
}

/**
 * This function gets the cluster to be used based on the starting cluster and the offset.
 */
static int fat16_get_cluster_for_offset(struct disk* disk, int starting_cluster, int offset) {
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / size_of_cluster_bytes;

    for (int i = 0; i < clusters_ahead; i++) {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        if (entry < 0) {
            return entry;
        }

        if (entry == SAMOS_FAT16_UNUSED || entry == SAMOS_FAT16_BAD_SECTOR ||
            (entry >= 0xFFF0 && entry < SAMOS_FAT16_EOC) || entry >= SAMOS_FAT16_EOC) {
            return -EIO;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;
out:
    return res;
}

static int fat16_read_internal_from_stream(struct disk* disk, struct disk_stream* stream, int cluster, int offset, int total, void* out) {
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0) {
        res = cluster_to_use;
        goto out;
    }

    int offset_from_cluster = offset % size_of_cluster_bytes;
    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    int bytes_left_in_cluster = size_of_cluster_bytes - offset_from_cluster;
    int total_to_read = total > bytes_left_in_cluster ? bytes_left_in_cluster : total;
    res = fat16_stream_read(disk, stream, starting_pos, total_to_read, out);
    if (res != SAMOS_ALL_OK) goto out;

    total -= total_to_read;
    if (total > 0) {
        // Still left to read
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset+total_to_read, total, (char*)out+total_to_read);
    }

out:
    return res;
}

static int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out) {
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

static int fat16_get_total_items_for_directory_cluster(struct disk* disk, int starting_cluster) {
    int res = 0;
    int offset = 0;
    struct fat_directory_item item;

    while(1) {
        res = fat16_read_internal(disk, starting_cluster, offset, sizeof(item), &item);
        if (res != SAMOS_ALL_OK)
            goto out;

        if (item.filename[0] == DIRECTORY_EMPTY) {
            break;
        }

        offset += sizeof(item);
    }

    res = offset / sizeof(item);
out:
    return res;
}

void fat16_free_directory(struct fat_directory* directory) {
    if (!directory)
        return;
    if (directory->item)
        kfree(directory->item);
    kfree(directory);
}


struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item) {
    int res = 0;
    struct fat_directory* directory = 0;
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY)) {
        res = -EINVARG;
        goto out;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory) goto out;

    int cluster = fat16_get_first_cluster(item);
    int total_items = fat16_get_total_items_for_directory_cluster(disk, cluster);
    if (total_items < 0) {
        res = total_items;
        goto out;
    }
    directory->total = total_items;
    int directory_size = directory->total * sizeof(struct fat_directory_item);
    if (directory_size > 0) {
        directory->item = kzalloc(directory_size);
        if(!directory->item) {
            res = -ENOMEM;
            goto out;
        }

        res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    }

out:
    if ((res != SAMOS_ALL_OK) && (directory)) {
        fat16_free_directory(directory);
        directory = 0;
    }
    return directory;
}

struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item) {
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if(!f_item) return 0;
    if (item->attribute & FAT_FILE_SUBDIRECTORY) {
        f_item->shared.directory = fat16_load_fat_directory(disk, item);
        if (!f_item->shared.directory) {
            kfree(f_item);
            return 0;
        }
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
    } else {
        f_item->type = FAT_ITEM_TYPE_FILE;
        f_item->shared.item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
        if (!f_item->shared.item) {
            kfree(f_item);
            return 0;
        }
    }
    return f_item;
}


/* Function which returns the fat_item for the item required, if present in the directory.
    name: string of the path */
struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name) {
    struct fat_item* f_item = 0;
    char tmp_filename[SAMOS_MAX_PATH];
    for (int i = 0; i < directory->total; i++) {
        if (directory->item[i].filename[0] == DIRECTORY_FREE ||
            directory->item[i].attribute == FAT_FILE_LONG_NAME ||
            directory->item[i].attribute & FAT_FILE_VOLUME_LABEL) {
            continue;
        }

        // Get the relative address and the extension of the item present so that it can be comapred to the path provided
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
        // Match when the names are equal.
        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
            // Found the path
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
            break;
        }
    }
    return f_item;
}

void fat16_fat_item_free(struct fat_item* item) {
    if (!item)
        return;

    if (item->type == FAT_ITEM_TYPE_DIRECTORY && item->shared.directory)
        fat16_free_directory(item->shared.directory);
    else if (item->shared.item)
        kfree(item->shared.item);

    kfree(item);
}

void fat16_item_free(struct fat_item* item) {
    fat16_fat_item_free(item);
}

/* Get the directory entry point */
struct fat_item* fat16_get_directory_entry(struct disk* disk, struct part_path* path) {
    // 
    struct fat_private* fat_private = disk->fs_private;
    struct fat_item* current_item = 0;
    // Look for an item in the given directory (based on the path provided)
    // Returns the first item found in the root
    struct fat_item* root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    // If no directory or file present, return NULL
    if (!root_item) goto out;
    struct part_path* next_part = path->next;
    current_item = root_item;
    // Iterte until the item is found
    while (next_part != 0) {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY) {
            fat16_item_free(current_item);
            current_item = 0;
            break;
        }

        struct fat_item* tmp_item = fat16_find_item_in_directory(disk, current_item->shared.directory, next_part->part);
        fat16_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }
out:
    return current_item;
}

void* fat16_open(struct disk* disk, struct part_path* path, FILE_MODE mode) {
    if (!disk || !path) {
        return ERROR(-EINVARG);
    }

    if (mode != FILE_MODE_READ) {   // Write support to be included later - for now read only
        return ERROR(-ERDONLY);
    }

    struct fat_file_descriptor* desc = 0;
    desc = kzalloc(sizeof(struct fat_file_descriptor));
    if(!desc) {
        return ERROR(-ENOMEM);
    }
    
    // Find the item defined in the path and see if it exists -> return the fat item if you find it
    desc->item = fat16_get_directory_entry(disk, path);
    if (!desc->item) {
        kfree(desc);
        return ERROR(-EIO);
    }
    if (desc->item->type != FAT_ITEM_TYPE_FILE) {
        fat16_item_free(desc->item);
        kfree(desc);
        return ERROR(-EINVARG);
    }

    // Set the position to the first character of the file / beginning of the file
    desc->pos = 0;
    return desc;
}

int fat16_read(struct disk* disk, void* desc, uint32_t size,  uint32_t nmemb, char* out_ptr) {
    int res = 0;
    struct fat_file_descriptor* fat_desc = desc;
    if (size == 0 || nmemb == 0) {
        goto out;
    }

    if (!fat_desc || !fat_desc->item || fat_desc->item->type != FAT_ITEM_TYPE_FILE || !out_ptr) {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item* item = fat_desc->item->shared.item;
    uint32_t offset = fat_desc->pos;
    uint32_t read_count = 0;

    for (uint32_t i = 0; i < nmemb; i++) {
        if (offset >= item->filesize || size > item->filesize - offset) {
            break;
        }

        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if (ISERR(res)) goto out;
        out_ptr += size;
        offset += size;
        read_count++;
    }

    fat_desc->pos = offset;
    res = read_count;
out:
    return res;
}

int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode) {
    int res = 0;
    struct fat_file_descriptor* desc = private;
    if (!desc || !desc->item) {
        res = -EINVARG;
        goto out;
    }

    struct fat_item* desc_item = desc->item;
    // Only seek within a file type, not a directory type
    if (desc_item->type != FAT_ITEM_TYPE_FILE) {
        res = -EINVARG;
        goto out;
    }
    struct fat_directory_item* ritem = desc_item->shared.item;
    uint32_t new_pos = desc->pos;

    switch(seek_mode) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_END:
            res = -EUNIMP;
            break;
        case SEEK_CUR:
            if (desc->pos > ritem->filesize) {
                res = -EIO;
                goto out;
            }
            if (offset > ritem->filesize - desc->pos) {
                res = -EIO;
                goto out;
            }
            new_pos = desc->pos + offset;
            break;
        default:
            res = -EINVARG;
            break;
    }

    if (res == SAMOS_ALL_OK) {
        if (new_pos > ritem->filesize) {
            res = -EIO;
            goto out;
        }
        desc->pos = new_pos;
    }
out:
    return res;
}

int fat16_stat(struct disk* disk, void* private, struct file_stat* stat) {
    int res = 0;
    struct fat_file_descriptor* desc = (struct fat_file_descriptor*) private;
    if (!desc || !desc->item || !stat) {
        res = -EINVARG;
        goto out;
    }

    struct fat_item* desc_item = desc->item;
    if(desc_item->type != FAT_ITEM_TYPE_FILE) {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item* ritem = desc_item->shared.item;
    stat->filesize = ritem->filesize;
    stat->flags = 0u;

    // Check and update if the file is read only
    if (ritem->attribute & FAT_FILE_READ_ONLY) {
        stat->flags |= FILE_STAT_READ_ONLY;
    }


out:
    return res;
}

static void fat16_free_file_descriptor(struct fat_file_descriptor* desc) {
    fat16_fat_item_free(desc->item);
    kfree(desc);
}

int fat16_close(void* private) {
    fat16_free_file_descriptor((struct fat_file_descriptor*) private);
    return 0;
}
