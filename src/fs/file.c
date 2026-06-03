#include "file.h"
#include "config.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "kernel.h"
#include "fat/fat16.h"
#include "disk/disk.h"
#include "string/string.h"

struct filesystem* filesystems[SAMOS_MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[SAMOS_MAX_FILE_DESCRIPTORS];

static struct filesystem** fs_get_free_filesystem() {
    int i = 0;
    for (i = 0; i < SAMOS_MAX_FILE_DESCRIPTORS; i++) {
        if (filesystems[i] == 0) {
            return &filesystems[i];
        }
    }
    return 0;
}


void fs_insert_filesystem(struct filesystem* filesystem) {
    struct filesystem** fs;
    if(filesystem == 0) {
        // Panic
        print("No filesystem found to insert.\n");
        while(1) {}
    }

    fs = fs_get_free_filesystem();
    if (!fs) {
        print("No space to insert filesystem.\n");
        while(1);
    }
    // Allocate the entry
    *fs = filesystem;
}


// Load the kernel required filesystems automatically
static void fs_static_load() {
    fs_insert_filesystem(fat16_init());

}


void fs_load() {
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}


void fs_int() {
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static void file_free_descriptor(struct file_descriptor* desc) {
    file_descriptors[desc->index - 1] = 0;
    kfree(desc);
}

static int file_new_descriptor(struct file_descriptor** desc_out) {
    int res = -ENOMEM;
    for (int i = 0; i < SAMOS_MAX_FILE_DESCRIPTORS; i++) {
        if(file_descriptors[i] == 0) {  // Look for empty slot in the descriptor to store
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            // Decriptors start counting at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }
    return res;
}

static struct file_descriptor* file_get_descriptor(int fd) {
    if (fd < 1 || fd >= SAMOS_MAX_FILE_DESCRIPTORS) {
        return 0;
    }
    // Descriptor index starts at 1
    int index = fd - 1;
    return file_descriptors[index];
}

struct filesystem* fs_resolve(struct disk* disk) {
    struct filesystem* fs = 0;
    for (int i = 0; i < SAMOS_MAX_FILESYSTEMS; i++) {
        if (filesystems[i]!=0 && filesystems[i]->resolve(disk) == 0) { // ensure not a null pointer and check he resolve function first
            fs = filesystems[i];
            break;
        }
    }
    return fs;
}

FILE_MODE file_get_mode_by_string(const char* str) {
    FILE_MODE mode = FILE_MODE_INVALID;
    if (!strncmp(str, "r", 1) == 0) {
        mode = FILE_MODE_READ;
    } else if (!strncmp(str, "w", 1) == 0) {
        mode = FILE_MODE_WRITE;
    } else if (!strncmp(str, "a", 1) == 0) {
        mode = FILE_MODE_APPEND;
    }
    return mode;
}

/* This function finds the location of the file using the drive, subdirectory info and opens it */
int fopen(const char* filename, const char* mode_str) {
    int res = 0;
    // Get the linked list for the path
    struct path_root* root_path = pathparser_parse(filename, NULL);
    if (!root_path) {
        res = -EINVARG;
        goto out;
    }
    // Check if it is a empty drive
    if (!root_path->first) {
        res = -EINVARG;
        goto out;
    }
    // CHeck if the drive number is valid
    struct disk* mydisk = disk_get(root_path->drive_no);
    if (!mydisk) {
        res = -EIO;
        goto out;
    }

    if (mydisk->filesystem) {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -EINVARG;
        goto out;
    }

    void* descriptor_private_data = mydisk->filesystem->open(mydisk, root_path->first, mode);
    if (ISERR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);
    if (res < 0)
        goto out;

    desc->filesystem = mydisk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = mydisk;
    res = desc->index;

out:
// fopen should not return negative values
    if (res < 0)
        res = 0;

    return res;
}

int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd){
    int res = 0;
    if (size == 0 || nmemb == 0 || fd < 1) {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -ENOMEM;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*)ptr);
out:
    return res;
}

/* Set the cursor or pointer to anywhere inside the file - lower implementation by the respective filesystem */
int fseek(int fd, int offset, FILE_SEEK_MODE whence) {
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }
    res = desc->filesystem->seek(desc->private, offset, whence);
out:
    return res;
}

int fstat(int fd, struct file_stat* stat) {
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->stat(desc->disk, desc->private, stat);
out:
    return res;
}

int fclose(int fd){
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }
    res = desc->filesystem->close(desc->private);
    if (res != 0) {
        goto out;
    }
    file_free_descriptor(desc);
out:
    return res;
}


