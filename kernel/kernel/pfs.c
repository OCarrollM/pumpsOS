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
    s->root_inode = 1; // Root dir
    ata_write_sector(PFS_SUPERBLOCK_SEC, sector); // Write the sectors

    // inode bitmap, all free to begin with
    memset(sector, 0, 512);
    sector[0] |= 0x01; // set bit 0 as sector 0
    sector[0] |= 0x02; 
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
    inode_t* root = (inode_t*)(sector + 1 * sizeof(inode_t));
    root->type = PFS_TYPE_DIR;
    root->size = PFS_BLOCK_SIZE;
    root->blocks[0] = 0;
    ata_write_sector(PFS_INODE_TABLE_SEC, sector);

    // root dir
    memset(sector, 0, 512);
    dirent_t* entries = (dirent_t*)sector;
    entries[0].inode = 1; strncpy(entries[0].name, ".", 28);
    entries[1].inode = 1; strncpy(entries[1].name, "..", 28); // These two set the typical root of . and ..
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

// create write and read
static bool pfs_dir_add(uint32_t dir_inode_num, const char* name, uint32_t inode_num) {
    inode_t dir;
    if (!pfs_read_inode(dir_inode_num, &dir)) return false;
    if (dir.type != PFS_TYPE_DIR) return false;

    uint32_t data_sec = sb.data_start_sec + dir.blocks[0];
    uint8_t sector[512];
    if (!ata_read_sector(data_sec, sector)) return false;

    dirent_t* entries = (dirent_t*)sector;
    for (int i = 0; i < 512 / (int)sizeof(dirent_t); i++) {
        if (entries[i].inode == 0) {
            // clean
            entries[i].inode = inode_num;
            strncpy(entries[i].name, name, 28);
            entries[i].name[27] = '\0';
            return ata_write_sector(data_sec, sector);
        }
    }
    return false;
}

// Create a file by allocating inode, init as empty file and dirent to root
int32_t pfs_create(const char* name) {
    int32_t ino = pfs_alloc_inode();
    if (ino < 1) return -1;

    inode_t node;
    memset(&node, 0, sizeof(node));
    node.type = PFS_TYPE_FILE;
    node.size = 0;
    if (!pfs_write_inode((uint32_t)ino, &node)) return -1;

    if (!pfs_dir_add(sb.root_inode, name, (uint32_t)ino)) return -1;

    return ino;
}

// write data to file
bool pfs_write_file(uint32_t ino, const uint8_t* data, uint32_t len) {
    inode_t node;
    if (!pfs_read_inode(ino, &node)) return false;
    if (node.type != PFS_TYPE_FILE) return false;

    // whole file write
    uint32_t blocks_needed = (len + PFS_BLOCK_SIZE - 1) / PFS_BLOCK_SIZE;
    if (blocks_needed > PFS_DIRECT_BLOCKS) return false;

    uint32_t written = 0;
    for (uint32_t b = 0; b < blocks_needed; b++) {
        // allocate block for this slot if it doesn't have one
        if (node.blocks[b] == 0) {
            int32_t blk = pfs_alloc_block();
            if (blk < 0) return false;
            node.blocks[b] = (uint32_t)blk;
        }
        // Fill sector buffer with this chunk
        uint8_t sector[512];
        memset(sector, 0, 512);
        uint32_t chunk = len - written;
        if (chunk > PFS_BLOCK_SIZE) chunk = PFS_BLOCK_SIZE;
        memcpy(sector, data + written, chunk);
        if (!ata_write_sector(sb.data_start_sec + node.blocks[b], sector)) return false;

        written += chunk;
    }

    node.size = len;
    return pfs_write_inode(ino, &node);
}

// read a file
int32_t pfs_read_file(uint32_t ino, uint8_t* out, uint32_t max) {
    inode_t node;
    if (!pfs_read_inode(ino, &node)) return -1;
    if (node.type != PFS_TYPE_FILE) return -1;

    uint8_t sector[512];
    uint32_t to_read = node.size;
    if (to_read > max) to_read = max;

    uint32_t read = 0;
    for (uint32_t b = 0; b < PFS_DIRECT_BLOCKS && read < to_read; b++) {
        if (node.blocks[b] == 0) break;
        if (!ata_read_sector(sb.data_start_sec + node.blocks[b], sector)) return -1;
        uint32_t chunk = to_read - read;
        if (chunk > PFS_BLOCK_SIZE) chunk = PFS_BLOCK_SIZE;
        memcpy(out + read, sector, chunk);
        
        read += chunk;
    }
    return (int32_t)read;
}

// Find a file by its name
int32_t pfs_lookup(const char* name) {
    inode_t root;
    if (!pfs_read_inode(sb.root_inode, &root)) return -1;
    uint8_t sector[512];
    if (!ata_read_sector(sb.data_start_sec + root.blocks[0], sector)) return -1;

    dirent_t* entries = (dirent_t*)sector;
    for (int i = 0; i < 512 / (int)sizeof(dirent_t); i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            return (int32_t)entries[i].inode;
        }
    }

    return -1; // not found
}

uint32_t pfs_get_root_inode(void) { return sb.root_inode; }
uint32_t pfs_data_sector(uint32_t block) { return sb.data_start_sec + block; }

// FILE IS DEVELOPED FROM TANEMBAUN OPERATING SYSTEMS V3