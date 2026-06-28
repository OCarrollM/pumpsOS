static int sys_write(int fd, const char* b, int n){int r;__asm__ volatile("int $0x80":"=a"(r):"a"(1),"b"(fd),"c"(b),"d"(n):"memory");return r;}
static int sys_read (int fd, char* b, int n){int r;__asm__ volatile("int $0x80":"=a"(r):"a"(5),"b"(fd),"c"(b),"d"(n):"memory");return r;}
static int sys_open (const char* p,int f){int r;__asm__ volatile("int $0x80":"=a"(r):"a"(6),"b"(p),"c"(f):"memory");return r;}
static int sys_close(int fd){int r;__asm__ volatile("int $0x80":"=a"(r):"a"(7),"b"(fd):"memory");return r;}
static void sys_exit(int c){__asm__ volatile("int $0x80"::"a"(0),"b"(c):"memory");__builtin_unreachable();}

static void puts_(const char* s){int n=0;while(s[n])n++;sys_write(1,s,n);}

int main(int argc, char** argv) {
    if (argc < 2) {
        puts_("usage: cat <file>\n");
        return 1;
    }
    int fd = sys_open(argv[1], 0);
    if (fd < 0) {
        puts_("cat: cannot open ");
        puts_(argv[1]);
        puts_("\n");
        return 1;
    }
    char buf[128];
    int n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        sys_write(1, buf, n);
    }
    sys_close(fd);
    return 0;
}

__attribute__((naked)) void _start(void) {
    __asm__ volatile(
        "movl 4(%esp), %eax\n"   /* eax = argv */
        "movl 0(%esp), %ebx\n"   /* ebx = argc */
        "pushl %eax\n"           /* push argv  (2nd arg) */
        "pushl %ebx\n"           /* push argc  (1st arg) */
        "call main\n"
        "movl %eax, %ebx\n"      /* main's return -> exit code in ebx */
        "movl $0, %eax\n"        /* SYS_EXIT */
        "int $0x80\n"
    );
}