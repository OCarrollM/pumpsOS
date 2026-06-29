#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#define MAX_ARGS 16

static int read_line(char* buf, int max) {
    int pos = 0;
    for (;;) {
        char c;
        int n = sys_read(0, &c, 1);
        if (n <= 0) continue;

        if (c == '\n') {
            putchar('\n');
            buf[pos] = '\0';
            return pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                sys_write(1, "\b \b", 3);
            }
        } else if (c >= 32 && c < 127) {
            if (pos < max - 1) {
                buf[pos++] = c;
                putchar(c);
            }
        }
    }
}

/* Split line in place into argv[], NUL-terminating tokens. Returns argc. */
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

/* Build "/<cmd>.elf" into out. */
static void build_path(const char* cmd, char* out, int max) {
    int i = 0;
    out[i++] = '/';
    for (int k = 0; cmd[k] && i < max - 6; k++) out[i++] = cmd[k];
    const char* ext = ".elf";
    for (int k = 0; ext[k] && i < max - 1; k++) out[i++] = ext[k];
    out[i] = '\0';
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    puts("pumpsOS shell. type 'help'.");

    char line[128];
    char path[140];

    for (;;) {
        printf("pumpsOS> ");

        int len = read_line(line, sizeof(line));
        if (len == 0) continue;

        char* args[MAX_ARGS];
        int n = tokenize(line, args, MAX_ARGS);
        if (n == 0) continue;

        /* Built-ins. */
        if (strcmp(args[0], "exit") == 0) {
            puts("bye");
            return 0;
        }
        if (strcmp(args[0], "help") == 0) {
            puts("pumpsOS shell");
            puts("  <program> [args]  - run a program");
            puts("  exit              - quit the shell");
            puts("  help              - this message");
            continue;
        }

        /* External command. */
        build_path(args[0], path, sizeof(path));

        int pid = sys_fork();
        if (pid < 0) {
            puts("fork failed");
            continue;
        }
        if (pid == 0) {
            sys_execve(path, args);
            printf("command not found: %s\n", args[0]);
            return 1;
        } else {
            int status = 0;
            sys_wait(&status);
        }
    }
}