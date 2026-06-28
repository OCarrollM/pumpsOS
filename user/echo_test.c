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

void _start(void) {
    static const char prompt[] = "type something and press enter:\n";
    sys_write(1, prompt, sizeof(prompt) - 1);

    char buf[128];
    /* Read until we see a newline, echoing as we go. */
    for (;;) {
        int n = sys_read(0, buf, sizeof(buf));
        if (n > 0) {
            sys_write(1, buf, n);          /* echo what we got */
            /* stop if the chunk contained a newline */
            for (int i = 0; i < n; i++) {
                if (buf[i] == '\n') {
                    static const char done[] = "you pressed enter, bye\n";
                    sys_write(1, done, sizeof(done) - 1);
                    sys_exit(0);
                }
            }
        }
    }
}