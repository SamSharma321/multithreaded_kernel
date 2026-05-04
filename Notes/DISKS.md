# Drives and Disks
1. We will be using ATA drives
2. These have 4 specifications for electrical connections of the cables:
    a. ATA Serial
    b. ATA Parallel
    c. ATAPI Serial
    d. ATAPI Parallel
3. Kernel Programmers usually do not care about whether the connection is serial or parallel.
4. ATA Drives (Advanced Technology Attachment) are standard interfaces for connecting storage devices such as HDD and SSD.
5. Types of drives: Primary and Secondary drives
6. LBA - Logical Block address - sector from which we read the data
7. Data is divided into sectors - of 512 bytes each


# File System:
* A Disk = giant array split into sectors
* Each sector = Logical Block Address (512 bytes)
* Disk itself has no concept of files - needs to be created
* File system = raw data + structure header
    - where the file is located
    - how big it is
    - How many files are present
* File system is on first sector of the harddisk
* Eg: FAT16. FAT32, NTFS, etc
* FAT = File Allocation Table (Microsoft Standard)
* Cluster = group of sectors
* Structure:
    - First sector is always boot sector + reserved 
    - First FAT - contains data on which clusters are free and which are taken
    - Second FAT (optional)
    - Root directory - explains the files, directory in the root + attributes (read/write permissions)
    - Data region comes last
* For a primary drive, the first sector is alwyas the boot sector followed by reserved sectors. Reserved sectors are to be intentionally ignored by the system.
* OS needs filesystem drivers so that they can read and write to disk.