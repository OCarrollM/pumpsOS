#include "initrd.h"
#include "heap.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// USTAR header (512 Bytes)
typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} __attribute__((packed)) ustar_header_t; 
// Yes this adds up to 512, I checked myself

typedef struct {
    char name[VFS_NAME_MAX];
    uint8_t* data;
    uint32_t size;
    bool is_directory;
} initrd_file_t;

static initrd_file_t* files = NULL;
static uint32_t file_count = 0;
static vfs_node_t* nodes = NULL;
static vfs_node_t* root_node = NULL;
static struct dirent dirent_buffer;

// Parse an octal as ASCII number (tar uses this)
static uint32_t parse_octal(const char* str, int len) {
    uint32_t result = 0;
    for (int i = 0; i < len && str[i]; i++) {
        if (str[i] < '0' || str[i] > '7') break;
        result = (result << 3) | (str[i] - '0');
    }
    return result;
}

// VFS Operations
static uint32_t initrd_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node->inode >= file_count) return 0;

    initrd_file_t* f = &files[node->inode];
    if (offset >= f->size) return 0;

    if (offset + size > f->size) {
        size = f->size - offset;
    }

    memcpy(buffer, f->data + offset, size);
    return size;
}

static struct dirent* initrd_readdir(vfs_node_t* node, uint32_t index) {
    if (node != root_node) return NULL;
    if (index >= file_count) return NULL;

    strncpy(dirent_buffer.name, files[index].name, VFS_NAME_MAX - 1);
    dirent_buffer.name[VFS_NAME_MAX - 1] = '\0';
    dirent_buffer.inode = index;

    return &dirent_buffer;
}

static vfs_node_t* initrd_finddir(vfs_node_t* node, const char* name) {
    if (node != root_node) return NULL;

    for (uint32_t i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &nodes[i];
        }
    }
    return NULL;
}

vfs_node_t* initrd_init(uint32_t location, uint32_t size) {
    uint8_t* data = (uint8_t*)location;
    uint32_t offset = 0;

    file_count = 0; // first pass
    while (offset < size) {
        ustar_header_t* hdr = (ustar_header_t*)(data + offset);

        if (hdr->name[0] == '\0') break;

        printf("[INITRD] PASS1 offset=%d name='%s' type='%c'\n",
           offset, hdr->name, hdr->type ? hdr->type : '?');

        if (memcmp(hdr->magic, "ustar", 5) != 0) {
            printf("WARNING: bad magic at offset %d\n", offset);
            break;
        }

        uint32_t fsize = parse_octal(hdr->size, 12);

        if (hdr->type == '0' || hdr->type == '\0') {
            file_count++;
        }

        offset += 512;
        offset += (fsize + 511) & ~511;
    }

    printf("Found %d files (%d bytes archive)\n", file_count, size);

    if (file_count == 0) {
        return NULL;
    }

    files = (initrd_file_t*)kmalloc(sizeof(initrd_file_t) * file_count);
    nodes = (vfs_node_t*)kmalloc(sizeof(vfs_node_t) * file_count);

    if (!files || !nodes) {
        printf("FATAL: kmalloc failed\n");
        return NULL;
    }

    offset = 0;
    uint32_t idx = 0;
    printf("Starting second pass, file_count=%d\n", file_count);

    while (offset < size && idx < file_count) {
        ustar_header_t* hdr = (ustar_header_t*)(data + offset);

        printf("Offset=%d, name[0]=0x%xm type='%c' (0x%x)\n", offset, (uint8_t)hdr->name[0], hdr->type ? hdr->type : '?', (uint8_t)hdr->type);
        if (hdr->name[0] == '\0') {
            printf("Empty name, breaking\n");
            break;
        }

        uint32_t fsize = parse_octal(hdr->size, 12);

        if (hdr->type == '0' || hdr->type == '\0') {
            const char* name = hdr->name;
            if (name[0] == '.' && name[1] == '/') name += 2;

            strncpy(files[idx].name, name, VFS_NAME_MAX - 1);
            files[idx].name[VFS_NAME_MAX - 1] = '\0';
            files[idx].data = data + offset + 512;
            files[idx].size = fsize;
            files[idx].is_directory = false;

            printf("File %d: '%s' size=%d\n", idx, files[idx].name, fsize);

            strncpy(nodes[idx].name, files[idx].name, VFS_NAME_MAX - 1);
            nodes[idx].name[VFS_NAME_MAX - 1] = '\0';
            nodes[idx].mask = 0;
            nodes[idx].uid = 0;
            nodes[idx].gid = 0;
            nodes[idx].flags = VFS_FILE;
            nodes[idx].inode = idx;
            nodes[idx].length = fsize;
            nodes[idx].impl = 0;
            nodes[idx].read = initrd_read;
            nodes[idx].write = NULL;        
            nodes[idx].open = NULL;
            nodes[idx].close = NULL;
            nodes[idx].readdir = NULL;
            nodes[idx].finddir = NULL;
            nodes[idx].ptr = NULL;

            idx++;
        } else {
            printf("Skipping type '%c'\n", hdr->type);
        }

        offset += 512;
        offset += (fsize + 511) & ~511;
    }
    printf("Second pass done, idx=%d\n", idx);

    root_node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    strncpy(root_node->name, "/", VFS_NAME_MAX - 1);
    root_node->name[1] = '\0';
    root_node->mask = 0;
    root_node->flags = VFS_DIRECTORY;
    root_node->inode = 0;
    root_node->length = 0;
    root_node->read = NULL;
    root_node->write = NULL;
    root_node->open = NULL;
    root_node->close = NULL;
    root_node->readdir = initrd_readdir;
    root_node->finddir = initrd_finddir;
    root_node->ptr = NULL;

    return root_node;
}

vfs_node_t* initrd_init_from_multiboot(multiboot_info_t* mboot) {
    if (!(mboot->flags & MULTIBBOOT_INFO_MODS)) {
        printf("No multiboot modules present\n");
        return NULL;
    }

    if (mboot->mods_count == 0) {
        printf("mods_count is zero\0");
        return NULL;
    }

    multiboot_module_t* mods = (multiboot_module_t*)(mboot->mods_addr + 0xC0000000);

    uint32_t mod_start = mods[0].mod_start;
    uint32_t mod_end = mods[0].mod_end;
    uint32_t size = mod_end - mod_start;

    printf("Module at phys 0x%x, size %d bytes\n", mod_start, size);

    return initrd_init(mod_start + 0xC0000000, size);
}