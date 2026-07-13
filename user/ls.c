// list user content
#include <stdio.h>
#include <sys/syscall.h>

int main(int argc, char** argv) {
    // default to /disk if no path given
    const char* path = (argc >= 2) ? argv[1] : "/disk";

    char name[64];
    unsigned int index = 0;
    int count = 0;

    while (sys_readdir(path, index, name) == 0) {
        printf("%s\n", name);
        index++;
        count++;
    }

    if (count == 0) {
        printf("(empty or not found: %s)\n", path);
    }
    return 0;
}