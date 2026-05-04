#include "io/io.h"
#include "stdint.h"
#include "disk.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"

#define TOTAL_DIVISIONS_PER_SECTOR          SAMOS_SECTOR_SIZE / 2

/* Structures */
static struct disk disk;

/* Function Implementations */
/* Note:
    0x1F0  data port
    0x1F1  error/features
    0x1F2  sector count
    0x1F3  LBA low
    0x1F4  LBA mid
    0x1F5  LBA high
    0x1F6  drive/head
    0x1F7  command when writing, status when reading
 */
static int disk_read_sector(int lba, int total, void *buf) {
    outb(0x1F6, (lba >> 24) | 0xE0);        //Select the master drive and pass part of LBA
    outb(0x1F2, total);                     // Total number of sectors to be read from the drive
    outb(0x1F3, (uint8_t)(lba & 0xff));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));      // Send more LBA
    outb(0x1F7, 0x20);                      // Read Command
    // 0x1F7 is the port number

    uint16_t* ptr = (uint16_t*)buf;
    for (int b = 0; b < total; b++) {       // Loop for total number of sectors
        // Wait till buffer is ready
        char c = insb(0x1F7);               // insb = input port to string byte (char)
                                            // Using port to read the drive
        while(!(c & 0x08)) {
            c = insb(0x1F7);                // Polling until buffer is ready for consumption
        }

        // Copy from harddsik to main memory
        for (int i = 0; i < TOTAL_DIVISIONS_PER_SECTOR; i++) {     // Each sector is 512 bytes, and we read 16 bts at a time -> 256 * 2 bytes
            *ptr = insw(0x1F0);
            ptr++;
        }
    }

    return 0;
}


/* Abstraction: Wrapper function for reading from drives */
void disk_search_and_init() {
    memset(&disk, 0, sizeof(disk));
    disk.type = SAMOS_DISK_TYPE_REAL;
    disk.sector_size = SAMOS_SECTOR_SIZE;
    disk.filesystem = fs_resolve(&disk);
}

struct disk* disk_get(int index) {
    if (index > 0) {        // Right noew we have only one disk
        return 0;
    }
    return &disk;
}


// lda is the port number
// buf should be of uint8_t type
int disk_read_block(struct disk* idisk, int lba, int total, void* buf) {
    if (idisk != &disk) {
        return -EIO;
    }
    return disk_read_sector(lba, total, buf);
}