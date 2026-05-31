// Freestanding (not assuming a hosted env, libc or main)

// This file is a regular C equiv to writing hello and then exiting
// usuall 2 lines.

// Point here is to prove the ELF loader works properly

static int sys_write(int fd, const char* buf, int len) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(1), "b"(fd), "c"(buf), "d"(len)
        : "memory"
    );
    return ret;
}

static void sys_exit(int code) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(0), "b"(code)
        : "memory"
    );
    __builtin_unreachable();
}

static const char msg[] = "Hello from ELF\n";

void _start(void) {
    sys_write(1, msg, sizeof(msg) - 1);
    sys_exit(0);
}