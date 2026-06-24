static int sys_write(int fd, const char* buf, int len) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(1), "b"(fd), "c"(buf), "d"(len) : "memory");
    return r;
}
static void sys_exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(0), "b"(code) : "memory");
    __builtin_unreachable();
}
static int sys_fork(void) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(2) : "memory");
    return r;
}
static int sys_wait(int* status) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(4), "b"(status) : "memory");
    return r;
}

void _start(void) {
    int pid = sys_fork();
    if (pid == 0) {
        static const char m[] = "child: running\n";
        sys_write(1, m, sizeof(m) - 1);
        sys_exit(42);
    } else {
        int status = 0;
        int reaped = sys_wait(&status);
        (void)reaped;
        static const char m[] = "parent: child reaped\n";
        sys_write(1, m, sizeof(m) - 1);
        sys_exit(0);
    }
}