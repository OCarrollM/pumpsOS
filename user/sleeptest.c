#include <stdio.h>
#include <sys/syscall.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    puts("sleeping for 2 seconds...");
    sys_sleep(2000);
    puts("awake");
    return 0;
}