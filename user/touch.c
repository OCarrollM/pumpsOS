// program for creating files
#include <stdio.h>
#include <sys/syscall.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        puts("usage: touch <path> <text>");
        return 1;
    }

    int fd = sys_open(argv[1], O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        printf("Open failed: %s\n", argv[1]);
        return 1;
    }

    // write the text
    int len = 0;
    while (argv[2][len]) len++;
    int n = sys_write(fd, argv[2], len);
    sys_close(fd);
    printf("wrote %d bytes to %s\n", n, argv[1]);
    return 0;
}