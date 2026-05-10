#ifndef FILE_H_
#define FILE_H_

#include "pparser.h"
#include "stdint.h"

typedef unsigned int FILE_SEEK_MODE;
typedef unsigned int FILE_MODE;

enum {
    SEEK_SET,
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
typedef int (*FS_READ_FUNCTION)(struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
// Check if the provided disk has the required sector data and its validity
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);
typedef int (*FS_SEEK_FUNCTION)(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);

// Each file system in the kernel should have this structure defined
struct filesystem {
    // Filesystem should return 0 from resolve if the provided disk is using its filesystem
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
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
int fopen(const char* filename, const char* mode_str);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
void fs_insert_filesystem(struct filesystem* filesystem);
struct filesystem* fs_resolve(struct disk* disk);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);

#endif