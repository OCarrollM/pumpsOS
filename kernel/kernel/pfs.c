// PumpsOS Filesystem main file
#include "pfs.h"
#include "../arch/i386/ata.h"
#include <string.h>
#include <stdio.h>

static superblock_t sb; // in-mem copy of superblock

// format blank disk first
void pfs_mkfs(void) {
    uint8_t sector[512];

    // superblock
    memset(sector, 0, 512); // Set 512 bytes into memory
    superblock_t* s = (superblock_t*)sector; // Call superblock
    s->magic = PFS_MAGIC;
    s->total_blocks = PFS_TOTAL_BLOCKS;
    s->total_inodes = PFS_TOTAL_INODES;
    s->inode_bitmap_sec = PFS_INODE_BITMAP_SEC;
    s->block_bitmap_sec = PFS_BLOCK_BITMAP_SEC;
    s->inode_table_sec = PFS_INODE_TABLE_SEC;
    s->data_start_sec = PFS_DATA_START_SEC;
    s->root_inode = 0; // Root dir
    ata_write_sector(PFS_SUPERBLOCK_SEC, sector); // Write the sectors

    // inode bitmap, all free to begin with
    memset(sector, 0, 512);
    sector[0] |= 0x01; // set bit 0 as sector 0
    ata_write_sector(PFS_INODE_BITMAP_SEC, sector);

    // Block bitmap, all free, allocate later
    memset(sector, 0, 512);
    sector[0] |= 0x01;
    ata_write_sector(PFS_BLOCK_BITMAP_SEC, sector);

    memset(sector, 0, 512);
    // zero out the remaining 7 blocks
    for (int i = 1; i < 8; i++) {
        ata_write_sector(PFS_BLOCK_BITMAP_SEC + i, sector);
    }

    // root inode, dir
    memset(sector, 0, 512);
    inode_t* root = (inode_t*)sector;
    root->type = PFS_TYPE_DIR;
    root->size = PFS_BLOCK_SIZE;
    root->blocks[0] = 0;
    ata_write_sector(PFS_INODE_TABLE_SEC, sector);

    // root dir
    memset(sector, 0, 512);
    dirent_t* entries = (dirent_t*)sector;
    entries[0].inode = 0; strncpy(entries[0].name, ".", 28);
    entries[1].inode = 0; strncpy(entries[1].name, "..", 28); // These two set the typical root of . and ..
    ata_write_sector(PFS_DATA_START_SEC + 0, sector);

    printf("MKFS complete: disk formatted\n"); // testing purposes
}

// Mount
bool pfs_mount(void) {
    uint8_t sector[512];
    if (!ata_read_sector(PFS_SUPERBLOCK_SEC, sector)) {
        printf("MOUNT: disk read failed\n");
        return false;
    }
    memcpy(&sb, sector, sizeof(superblock_t));

    if (sb.magic != PFS_MAGIC) {
        printf("MOUNT: bad magic 0x%x (disk not formatted)\n", sb.magic);
        return false;
    }

    printf("Mounted: %d blocks, %d inodes, root inode %d\n", sb.total_blocks, sb.total_inodes, sb.root_inode);
    return true;
}

// Read inode n into *out
bool pfs_read_inode(uint32_t n, inode_t* out) {
    if (n >= PFS_TOTAL_INODES) return false;
    uint8_t sector[512];
    uint32_t sec = sb.inode_table_sec + (n / 8);
    if (!ata_read_sector(sec, sector)) return false;
    memcpy(out, sector + (n % 8) * sizeof(inode_t), sizeof(inode_t));
    return true;
}

// Write inode n from *in
bool pfs_write_inode(uint32_t n, const inode_t* in) {
    if (n >= PFS_TOTAL_INODES) return false;
    uint8_t sector[512];
    uint32_t sec = sb.inode_table_sec + (n / 8);
    if (!ata_read_sector(sec, sector)) return false;
    memcpy(sector + (n % 8) * sizeof(inode_t), in, sizeof(inode_t));
    return ata_write_sector(sec, sector);
}

// find a free inode, mark used then return its number
int32_t pfs_alloc_inode(void) {
    uint8_t bitmap[512];
    if (!ata_read_sector(sb.inode_bitmap_sec, bitmap)) return -1;

    for (uint32_t i = 0; i < PFS_TOTAL_INODES; i++) {
        uint32_t byte = i / 8, bit = i % 8;
        if (!(bitmap[byte] & (1 << bit))) {
            bitmap[byte] |= (1 << bit);
            ata_write_sector(sb.inode_bitmap_sec, bitmap);
            return (int32_t)i;
        }
    }
    return -1;
}

// find a free data block, mark used, then return number
int32_t pfs_alloc_block(void) {
    uint8_t bitmap[512];
    for (uint32_t s = 0; s < 8; s++) {
        if (!ata_read_sector(sb.block_bitmap_sec + s, bitmap)) return -1;
        for (uint32_t i = 0; i < 512 * 8; i++) {
            uint32_t global = s * 512 * 8 + i;
            if (global >= PFS_TOTAL_BLOCKS) return -1;
            uint32_t byte = i / 8, bit = i % 8;
            if (!(bitmap[byte] & (1 << bit))) {
                bitmap[byte] |= (1 << bit);
                ata_write_sector(sb.block_bitmap_sec + s, bitmap);
                return (int32_t)global;
            }
        }
    }
    return -1;
}