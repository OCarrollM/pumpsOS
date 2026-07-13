// A user side syscall file for write read exec, etc. 
// Basically real functions rather than being in the kernel

#include <stdint.h>
#include <sys/syscall.h>

struct thread_start { void (*fn)(void*); void* arg; };

static void thread_trampoline(void* packed) {
    struct thread_start* ts = (struct thread_start*)packed;
    ts->fn(ts->arg);
    sys_thread_exit();
    for(;;){};
}

int thread_create(void (*fn)(void*), void* arg) {
    static struct thread_start ts;  
    ts.fn = fn;
    ts.arg = arg;
    return sys_thread_create((uint32_t)thread_trampoline, (uint32_t)&ts);
}

int sys_write(int fd, const void* buf, int len) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(len) : "memory");
    return r;
}
int sys_read(int fd, void* buf, int len) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(len) : "memory");
    return r;
}
int sys_open(const char* path, int flags) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(SYS_OPEN), "b"(path), "c"(flags) : "memory");
    return r;
}
int sys_close(int fd) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_CLOSE), "b"(fd) : "memory");
    return r;
}
int sys_fork(void) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_FORK) : "memory");
    return r;
}
int sys_execve(const char* path, char* const argv[]) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(SYS_EXECVE), "b"(path), "c"(argv) : "memory");
    return r;
}
int sys_wait(int* status) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_WAIT), "b"(status) : "memory");
    return r;
}
void sys_exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code) : "memory");
    __builtin_unreachable();
}

int sys_sleep(int ms) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_SLEEP), "b"(ms) : "memory");
    return r;
}

void exit(int code) {
    sys_exit(code);
    __builtin_unreachable();
}

int sys_thread_create(unsigned int entry, unsigned int arg) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(SYS_THREAD_CREATE), "b"(entry), "c"(arg) : "memory");
    return r;
}

void sys_thread_exit(void) {
    __asm__ volatile("int $0x80" : : "a"(SYS_THREAD_EXIT) : "memory");
    __builtin_unreachable();
}

int sys_readdir(const char* path, unsigned int index, char* name_out) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_READDIR), "b"(path), "c"(index), "d"(name_out) : "memory");
    return r;
}