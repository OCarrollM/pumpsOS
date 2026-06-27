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
static int sys_read(int fd, char* buf, int len) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(5), "b"(fd), "c"(buf), "d"(len) : "memory");
    return r;
}
static int sys_open(const char* path, int flags) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(6), "b"(path), "c"(flags) : "memory");
    return r;
}

void _start(void) {
    static const char path[] = "/readme.txt";
    int fd = sys_open(path, 0);
    if (fd < 0) {
        static const char e[] = "open failed\n";
        sys_write(1, e, sizeof(e) - 1);
        sys_exit(1);
    }

    char buf[128];
    int n = sys_read(fd, buf, sizeof(buf));
    if (n > 0) {
        sys_write(1, buf, n);
    } else {
        static const char e[] = "read returned nothing\n";
        sys_write(1, e, sizeof(e) - 1);
    }
    sys_exit(0);
}