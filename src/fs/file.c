#include "file.h"
#include "config.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "kernel.h"

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
    // fs_insert_filesystem(fat16_init());

}


void fs_load() {
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}


void fs_int() {
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
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
    if (fd < 1 || fd > SAMOS_MAX_FILE_DESCRIPTORS) {
        return 0;
    }
    // Descriptor index starts at 1
    int index = fd - 1;
    return file_descriptors[index];
}

struct filesystem* fs_resolve(struct disk* disk) {
    struct filesystem* fs = 0;
    for (int i = 0; i < SAMOS_MAX_FILESYSTEMS; i++) {
        if (filesystems[i]!=0 && filesystems[i]->resolve(disk)==0) { // ensure not a null pointer and check he resolve function first
            fs = filesystems[i];
            break;
        }
    }
    return fs;
}

int fopen(const char* filename, const char* mode) {
    return -EIO;
}

