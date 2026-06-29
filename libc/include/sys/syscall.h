#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_FORK 2
#define SYS_EXECVE 3
#define SYS_WAIT 4
#define SYS_READ 5
#define SYS_OPEN 6
#define SYS_CLOSE 7

int sys_write(int fd, const void* buf, int len);
int sys_read(int fd, void* buf, int len);
int sys_open(const char* path, int flags);
int sys_close(int fd);
int sys_fork(void);
int sys_execve(const char* path, char* const argv[]);
int sys_wait(int* status);
void sys_exit(int code);

#endif