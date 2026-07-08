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
#define SYS_SLEEP 8
#define SYS_THREAD_CREATE 9
#define SYS_THREAD_EXIT 10

int sys_write(int fd, const void* buf, int len);
int sys_read(int fd, void* buf, int len);
int sys_open(const char* path, int flags);
int sys_close(int fd);
int sys_fork(void);
int sys_execve(const char* path, char* const argv[]);
int sys_wait(int* status);
void sys_exit(int code);
int sys_sleep(int ms);
void exit(int code);
int main(int, char**);
int sys_thread_create(unsigned int entry, unsigned int arg);
void sys_thread_exit(void);

#endif