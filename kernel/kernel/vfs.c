#include "vfs.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_MOUNTS 4

vfs_node_t* vfs_root = NULL;

typedef struct {
    char prefix[VFS_NAME_MAX];
    vfs_node_t* root;
    bool used;
} mount_t;
static mount_t mounts[MAX_MOUNTS];

uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node && node->write) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

void vfs_open(vfs_node_t* node) {
    if (node && node->open) {
        node->open(node);
    }
}

void vfs_close(vfs_node_t* node) {
    if (node && node->close) {
        node->close(node);
    }
}

struct dirent* vfs_readdir(vfs_node_t* node, uint32_t index) {
    if (node && (node->flags & 0x7) == VFS_DIRECTORY && node->readdir) {
        return node->readdir(node, index);
    }
    return NULL;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, const char* name) {
    if (node && (node->flags & 0x7) == VFS_DIRECTORY && node->finddir) {
        return node->finddir(node, name);
    }
    return NULL;
}

// Walk a path (such as one/two/three) and split at the slash calling findir on each component
vfs_node_t* vfs_lookup(const char* path) {
    if (!vfs_root || !path) {
        return NULL;
    }

    if (path[0] == '/') {
        path++;
    }

    if (path[0] == '\0') {
        return vfs_root; // This is the root
    }

    char first[VFS_NAME_MAX];
    int i = 0;
    const char* p = path;
    while (*p && *p != '/' && i < VFS_NAME_MAX - 1) first[i++] = *p++;
    first[i] = '\0';

    // is the first component a mount point?
    for (int m = 0; m < MAX_MOUNTS; m++) {
        if (mounts[m].used && strcmp(mounts[m].prefix, first) == 0) {
            if (*p == '/') p++;
            if (*p == '\0') return mounts[m].root;
            return vfs_finddir(mounts[m].root, p);
        }
    }

    vfs_node_t* current = vfs_root;
    char component[VFS_NAME_MAX];
    const char* path2 = path;

    while (*path2 && current) {
        int j = 0;
        while (*path2 && *path2 != '/' && j < VFS_NAME_MAX - 1) {
            component[j++] = *path2++;
        }
        component[j] = '\0';

        if (*path2 == '/') {
            path2++;
        }

        current = vfs_finddir(current, component);
    }
    return current;
}

void vfs_mount(const char* prefix, vfs_node_t* root) {
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mounts[i].used) {
            strncpy(mounts[i].prefix, prefix, VFS_NAME_MAX - 1);
            mounts[i].root = root;
            mounts[i].used = true;
            return;
        }
    }
}