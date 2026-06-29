#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#else
#include <sys/syscall.h>
#endif

int putchar(int ic) {
    char c = (char) ic;
    #if defined(__is_libk)
        terminal_write(&c, sizeof(c));
    #else
        sys_write(1, &c, 1); // user side
    #endif
        return ic;
}