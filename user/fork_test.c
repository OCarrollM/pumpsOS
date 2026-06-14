// Testing the forking syscall I made
// Both lines need t appear in print for it t work

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

static int sys_fork(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(2) // Fork
        : "memory"
    );
    return ret;
}

// Helpers since libc isn't available
static void str_reverse(char* s, int n) {
    for (int i = 0; i < n / 2; i++) {
        char t = s[i]; s[i] = s[n-1-i]; s[n-1-i] = t;
    }
}

// int to string
static int itoa(int v, char* buf) {
    int n = 0;
    int neg = (v < 0);
    if (neg) v = -v;
    if (v == 0) { buf[n++] = '0'; }
    while (v > 0) { buf[n++] = '0' + (v % 10); v /= 10; }
    if (neg) buf[n++] = '-';
    str_reverse(buf, n);
    return n;
}

// entry
void _start(void) {
    // int pid = sys_fork();
    
    // if (pid == 0) {
    //     static const char msg[] = "child: hello from fork\n";
    //     sys_write(1, msg, sizeof(msg) - 1);
    //     sys_exit(0);
    // } else if (pid > 0) {
    //     char buf[64];
    //     static const char prefix[] = "parent: child pid is ";
    //     int i = 0;
    //     for (unsigned k = 0; k < sizeof(prefix) - 1; k++) buf[i++] = prefix[k];
    //     i += itoa(pid, buf + 1);
    //     buf[i++] = '\n';
    //     sys_write(1, buf, i);
    //     sys_exit(0);
    // } else {
    //     static const char err[] = "fork failed\n";
    //     sys_write(1, err, sizeof(err) - 1);
    //     sys_exit(1);
    // }
    static const char msg[] = "fork_test started\n";
    sys_write(1, msg, sizeof(msg) - 1);
    sys_exit(0);
}