#include <stdio.h>
#include <sys/syscall.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        puts("usage: cat <file>");
        return 1;
    }
    int fd = sys_open(argv[1], 0);
    if (fd < 0) {
        printf("cat: cannot open %s\n", argv[1]);
        return 1;
    }
    char buf[128];
    int n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        sys_write(1, buf, n);
    }
    sys_close(fd);
    return 0;
}