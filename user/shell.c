static int sys_write(int fd, const char* buf, int len) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(1), "b"(fd), "c"(buf), "d"(len) : "memory");
    return r;
}
static int sys_read(int fd, char* buf, int len) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r)
        : "a"(5), "b"(fd), "c"(buf), "d"(len) : "memory");
    return r;
}
static int sys_fork(void) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(2) : "memory");
    return r;
}
static int sys_execve(const char* path) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(3), "b"(path) : "memory");
    return r;
}
static int sys_wait(int* status) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(4), "b"(status) : "memory");
    return r;
}
static void sys_exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(0), "b"(code) : "memory");
    __builtin_unreachable();
}

/* ---- hand-rolled string helpers (future libc residents) ---- */

static unsigned slen(const char* s) {
    unsigned n = 0;
    while (s[n]) n++;
    return n;
}

static int seq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;   /* both ended together */
}

/* Write a NUL-terminated string to stdout. */
static void puts_(const char* s) {
    sys_write(1, s, slen(s));
}

/* ---- line input ----
 *
 * keyboard_read blocks for the first char, returns 1+ chars at a
 * time, and does NOT echo. So we loop, echo each char ourselves,
 * handle backspace, and stop at newline. Returns line length.
 */
static int read_line(char* buf, int max) {
    int pos = 0;
    for (;;) {
        char c;
        int n = sys_read(0, &c, 1);
        if (n <= 0) continue;        /* spurious wake; keep waiting */

        if (c == '\n') {
            sys_write(1, "\n", 1);   /* echo the newline */
            buf[pos] = '\0';
            return pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                sys_write(1, "\b \b", 3);  /* erase on screen */
            }
        } else if (c >= 32 && c < 127) {
            if (pos < max - 1) {
                buf[pos++] = c;
                sys_write(1, &c, 1);  /* echo the typed char */
            }
        }
        /* other control chars ignored */
    }
}

/* ---- command resolution: "hello" -> "/hello.elf" ---- */

static void build_path(const char* cmd, char* out, int max) {
    int i = 0;
    out[i++] = '/';
    for (int k = 0; cmd[k] && i < max - 6; k++) out[i++] = cmd[k];
    /* append ".elf" */
    const char* ext = ".elf";
    for (int k = 0; ext[k] && i < max - 1; k++) out[i++] = ext[k];
    out[i] = '\0';
}

/* ---- built-ins ---- */

static int run_builtin(const char* cmd) {
    if (seq(cmd, "exit")) {
        puts_("shell exiting\n");
        sys_exit(0);
    }
    if (seq(cmd, "help")) {
        puts_("pumpsOS shell\n");
        puts_("  type a program name to run it (e.g. hello)\n");
        puts_("  exit  - quit the shell\n");
        puts_("  help  - this message\n");
        return 1;   /* handled */
    }
    return 0;       /* not a built-in */
}

static int tokenize(char* line, char** argv, int max) {
    int argc = 0;
    char* p = line;
    while (*p && argc < max - 1) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    argv[argc] = 0;
    return argc;
}

/* ---- main loop ---- */

void _start(void) {
    puts_("pumpsOS shell. type 'help'.\n");

    char line[128];
    char path[140];

    for (;;) {
        puts_("pumpsOS> ");

        int len = read_line(line, sizeof(line));
        if (len == 0) continue;          /* empty line */

        char* argv[MAX_ARGS];
        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc == 0) continue;
        if (run_builtin(line)) continue; /* exit/help handled */

        /* Built-ins check argv[0]. */
        if (seq(argv[0], "exit")) { puts_("bye\n"); sys_exit(0); }
        if (seq(argv[0], "help")) { /* ... help text ... */ continue; }

        /* External: build /<argv[0]>.elf, fork, exec with argv. */
        build_path(argv[0], path, sizeof(path));

        int pid = sys_fork();
        if (pid < 0) { puts_("fork failed\n"); continue; }
        if (pid == 0) {
            sys_execve(path, argv);     /* now passes argv */
            puts_("command not found: "); puts_(argv[0]); puts_("\n");
            sys_exit(1);
        } else {
            int status = 0;
            sys_wait(&status);
        }
    }
}