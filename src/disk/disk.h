#ifndef DISK_H_
#define DISK_H_
/* NOTE: QEMU emulates disks, including hard disks, SSDs, USB flash drives, 
and SD cards, allowing virtual machines to interact with storage as if it were 
physical hardware. It translates disk I/O requests from the guest OS into file 
operations on the host system, typically using file-based images */

#include "fs/file.h"

// Represets a real physical hard disk
#define SAMOS_DISK_TYPE_REAL 0

typedef unsigned int SAMOS_DISK_TYPE;

struct disk {
    SAMOS_DISK_TYPE type;
    int sector_size;
    struct filesystem* filesystem;      // Filesystem for th created disk
};


void disk_search_and_init();
struct disk* disk_get(int index);
int disk_read_block(struct disk* idisk, int lba, int total, void* buf);

#endif