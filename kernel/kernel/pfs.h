// PumpsFS (file system) an inode-based file system
#ifndef KERNEL_PFS_H
#define KERNEL_PFS_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PFS_MAGIC 0x50465321 // generated online it becomes "PFS!"
#define PFS_BLOCK_SIZE 512
#define PFS_TOTAL_BLOCKS 32768
#define PFS_TOTAL_INODES 4096
#define PFS_DIRECT_BLOCKS 12

// On disk region layout
#define PFS_SUPERBLOCK_SEC 0
#define PFS_INODE_BITMAP_SEC 1
#define PFS_BLOCK_BITMAP_SEC 2
#define PFS_INODE_TABLE_SEC 10
#define PFS_DATA_START_SEC 522

#define PFS_TYPE_FREE 0
#define PFS_TYPE_FILE 1
#define PFS_TYPE_DIR 2

// Super block - describes the file system, sort of copied from an old FAT16 coursework i did in uni
typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t inode_bitmap_sec;
    uint32_t block_bitmap_sec;
    uint32_t inode_table_sec;
    uint32_t data_start_sec;
    uint32_t root_inode;
} superblock_t;

// Inode -- 64 bytes, 8 per sector
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t blocks[PFS_DIRECT_BLOCKS];
    uint32_t reserved[2];
} inode_t;

// dir entry
typedef struct {
    uint32_t inode;
    char name[28];
} dirent_t;

void pfs_mkfs(void);
bool pfs_mount(void);
bool pfs_read_inode(uint32_t n, inode_t* out);
bool pfs_write_inode(uint32_t n, const inode_t* in);
int32_t pfs_alloc_inode(void);
int32_t pfs_alloc_block(void);
int32_t pfs_create(const char* name);
bool pfs_write_file(uint32_t ino, const uint8_t* data, uint32_t len);
int32_t pfs_read_file(uint32_t ino, uint8_t* out, uint32_t max);
int32_t pfs_lookup(const char* name);
uint32_t pfs_get_root_inode(void);
uint32_t pfs_data_sector(uint32_t block);

// More stuff to come

#endif