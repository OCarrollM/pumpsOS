#include "vfs.h"
#include <stdio.h>
#include <string.h>

vfs_node_t* vfs_root = NULL;

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

    vfs_node_t* current = vfs_root;
    char component[VFS_NAME_MAX];

    while (*path && current) {
        int i = 0;
        while (*path && *path != '/' && i < VFS_NAME_MAX - 1) {
            component[i++] = *path++;
        }
        component[i] = '\0';

        if (*path == '/') {
            path++;
        }

        current = vfs_finddir(current, component);
    }
    return current;
}