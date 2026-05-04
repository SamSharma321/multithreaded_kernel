#ifndef FILE_H_
#define FILE_H_

#include "pparser.h"

typedef unsigned int FILE_SEEK_MODE;
typedef unsigned int FILE_MODE;

enum {
    SEEK_GET,
    SEEK_CUR,
    SEEK_END
};


enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID,
    
};


struct disk;

// Function pointer typedef retrntype (* function_ptr_name) (params...)
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct part_path* path, FILE_MODE mode);
// Check if the provided disk has the required sector data and its validity
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

// Each file system in the kernel should have this structure defined
struct filesystem {
    // Filesystem should return 0 from resolve if the provided disk is using its filesystem
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    char name[20];
};


struct file_descriptor {
    // The descriptor index
    int index;
    struct filesystem* filesystem;
    // Private data for internal file descriptor - the filesystem uses this info to locate the file structure
    void* private;
    // The disk that the file descriptor should be used on
    struct disk* disk;
};

/* Function Prototypes */
void fs_init();
int fopen(const char* filename, const char* mode);
void fs_insert_filesystem(struct filesystem* filesystem);
struct filesystem* fs_resolve(struct disk* disk);

#endif