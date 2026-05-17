#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_NAME_MAX 64

// Node types
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEVICE  0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_MOUNTPOINT  0x08

struct vfs_node;

typedef uint32_t (*read_func_t)(struct vfs_node*, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef uint32_t (*write_func_t)(struct vfs_node*, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef void (*open_func_t)(struct vfs_node*);
typedef void (*close_func_t)(struct vfs_node*);
typedef struct dirent* (*readdir_func_t)(struct vfs_node*, uint32_t index);
typedef struct vfs_node* (*finddir_func_t)(struct vfs_node*, const char* name);

// A dir entry returned by readdir
struct dirent {
    char name[VFS_NAME_MAX];
    uint32_t inode;
};

// A VFS node represents a file, dir, or a device

typedef struct vfs_node {
    char name[VFS_NAME_MAX];        // Filename
    uint32_t mask;                  // Perms
    uint32_t uid;                   // Owner
    uint32_t gid;                   // Group
    uint32_t flags;                 // Node type
    uint32_t inode;                 // File system specific id
    uint32_t length;                // Size in bytes
    uint32_t impl;                  // Filesystem specific use

    // Function pointers
    read_func_t read;
    write_func_t write;
    open_func_t open;
    close_func_t close;
    readdir_func_t readdir;
    finddir_func_t finddir;

    struct vfs_node* ptr; // Used by mount points
} vfs_node_t;

// Root of the file system
extern vfs_node_t* vfs_root;

// Public interface
uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void vfs_open(vfs_node_t* node);
void vfs_close(vfs_node_t* node);
struct dirent* vfs_readdir(vfs_node_t* node, uint32_t index);
vfs_node_t* vfs_finddir(vfs_node_t* node, const char* name);

// Look up a node by path and return NULL if not found
vfs_node_t* vfs_lookup(const char* path);

#endif

//! VFS DESIGN FROM JAMESMALLOY.CO.UK