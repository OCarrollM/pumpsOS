#include <stdio.h>
#include <sys/syscall.h>

static void worker(void* arg) {
    for (int i = 0; i < 10; i++) {
        printf("[WORKER] %d\n", i);
        for (volatile int d = 0; d < 20000000; d++) {}
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    thread_create(worker, 0);
    for (int i = 0; i < 3; i++) {
        printf("[MAIN] %d\n", i);
        for (volatile int d = 0; d < 20000000; d++) {}
    }
    puts("main done (worker still running)");
    return 0;
}