// bridge from PFS to VFS layer
#include "pfs.h"
#include "vfs.h"
#include "../arch/i386/ata.h"
#include <string.h>
#include <stdio.h>

// vfs read on a pfs file
static uint32_t pfs_vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // read whole file, ignore the offect
    uint8_t tmp[512 * 12]; // 6KB
    int32_t n = pfs_read_file(node->inode, tmp, sizeof(tmp));
    if (n < 0) return 0;

    // Apply offset
    if (offset >= (uint32_t)n) return 0;
    uint32_t avail = (uint32_t)n - offset;
    uint32_t to_copy = size < avail ? size : avail;
    memcpy(buffer, tmp + offset, to_copy);
    return to_copy;
}

// vfs write on pfs file
static uint32_t pfs_vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    (void)offset;
    printf("[pfs_write] inode=%d size=%d\n", node->inode, size);
    bool ok = pfs_write_file(node->inode, buffer, size);
    printf("[pfs_write] pfs_write_file returned %d\n", ok);
    return ok ? size : 0;
}

// Fill vfs node for a pfs file with a given inode and name
static void pfs_make_node(vfs_node_t* node, uint32_t inode, const char* name) {
    memset(node, 0, sizeof(vfs_node_t));
    strncpy(node->name, name, VFS_NAME_MAX - 1);
    node->inode = inode;
    node->flags = VFS_FILE;
    node->read = pfs_vfs_read;
    node->write = pfs_vfs_write;

    // set length from the pfs inode
    inode_t pnode;
    if (pfs_read_inode(inode, &pnode)) {
        node->length = pnode.size;
    }
}

// find dir on the pfs root
// look up a name and return vfs node
static vfs_node_t pfs_lookup_node; // static node for now

static vfs_node_t* pfs_vfs_finddir(vfs_node_t* node, const char* name) {
    (void)node;
    int32_t inode = pfs_lookup(name);
    if (inode < 0) return NULL;
    pfs_make_node(&pfs_lookup_node, (uint32_t)inode, name);
    return &pfs_lookup_node;
}

// read dir on the pfs node
static struct dirent pfs_readdir_buf;

static struct dirent* pfs_vfs_readdir(vfs_node_t* node, uint32_t index) {
    (void)node;
    inode_t root;
    if (!pfs_read_inode(pfs_get_root_inode(), &root)) return NULL;
    uint8_t sector[512];
    if (!ata_read_sector(pfs_data_sector(root.blocks[0]), sector)) return NULL;

    dirent_t* entries = (dirent_t*)sector;
    uint32_t count = 0;
    for (int i = 0; i < 512 / (int)sizeof(dirent_t); i++) {
        if (entries[i].inode != 0) {
            if (count == index) {
                strncpy(pfs_readdir_buf.name, entries[i].name, 27);
                pfs_readdir_buf.name[27] = '\0';
                pfs_readdir_buf.inode = entries[i].inode;
                return &pfs_readdir_buf;
            }
            count++;
        }
    }
    return NULL;
}

static vfs_node_t* pfs_vfs_create(vfs_node_t* dir, const char* name) {
    (void)dir;
    if (pfs_lookup(name) >= 0) return NULL;

    int32_t ino = pfs_create(name);
    if (ino < 0) return NULL;

    pfs_make_node(&pfs_lookup_node, (uint32_t)ino, name);
    return &pfs_lookup_node;
}

// root VFS node
static vfs_node_t pfs_root_node;

vfs_node_t* pfs_vfs_init(void) {
    memset(&pfs_root_node, 0, sizeof(vfs_node_t));
    strncpy(pfs_root_node.name, "/", VFS_NAME_MAX - 1);
    pfs_root_node.flags = VFS_DIRECTORY;
    pfs_root_node.finddir = pfs_vfs_finddir;
    pfs_root_node.readdir = pfs_vfs_readdir;
    pfs_root_node.create = pfs_vfs_create;
    return &pfs_root_node;
}