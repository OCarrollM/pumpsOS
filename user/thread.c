#include <stdio.h>
#include <sys/syscall.h>

static void worker(void* arg) {
    const char* tag = (const char*)arg;
    for (int i = 0; i < 5; i++) {
        printf("[%s] %d\n", tag, i);
        for (volatile int d = 0; d < 20000000; d++) { }
    }
    /* returns -> trampoline -> sys_thread_exit */
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    puts("main: spawning a thread");
    thread_create(worker, "WORKER");

    /* main thread keeps working too */
    for (int i = 0; i < 5; i++) {
        printf("[MAIN] %d\n", i);
        for (volatile int d = 0; d < 20000000; d++) { }
    }
    puts("main: done");
    /* NOTE: main returning while worker may still run is a lifetime
     * issue (step 2). For the test, main's loop count matches worker's
     * so they finish around the same time. */
    return 0;
}