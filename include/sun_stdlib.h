/*
 * sun_stdlib.h 95/06/07
 *
 * Copyright 1994, BBW
 *
 */
/*
 * Copyright 1992-1994 Rudolf Koenig.
 * sun_stdlib.h 
 *
 * This header file is redundant/wrong for architectures that have sane 
 * header files. Grrr.
 */
#ifndef __sun_stdlib_h
#define __sun_stdlib_h
#if defined(FILE)

#if !defined(STDIO_WRITE_NOW)

extern int fputs(char *, FILE *);
extern int fseek(FILE *, int, int);
extern int fclose(FILE *);
extern int fread(char *, int, int, FILE *);
extern int fwrite(char *, int, int, FILE *);
extern int fflush(FILE *);
extern int fprintf(FILE *, const char *, ... );
extern void rewind(FILE *);
extern void setvbuf(FILE *stream, char *buf, int type, int size);
extern void setbuffer(FILE *stream, char *buf, int size);

extern int _filbuf( FILE * );
extern int _flsbuf(unsigned char, FILE*);
#else
extern int _fill_buffer_( FILE * );
extern int _flush_buffer_(int, FILE*);
#endif

#endif

extern int puts(char *);

#if !defined(STDIO_WRITE_NOW)
extern int printf( const char *, ... );
#endif


extern int getpid(void);
extern int getppid(void);
extern int pipe(int *);

#if 1
extern char *sprintf( char *, const char *, ... );
extern char *vsprintf( char *, const char *, ... );
#endif
extern int vfork(void);
extern int fork(void);
extern int close(int);
extern int dup(int);

#ifndef __sys_unistd_h /* GNUCC has another imagination about this */
extern int execl(char *, ... );
extern int execv(char *, char *[]);
extern int execle(char *, ... );
extern int execlp(char *, ... );
extern int execvp(char *, char *[]);
extern int setuid(int);
#endif

extern int seteuid(int);

extern int getpgrp(int);
extern int setpgrp(int, int);

extern int gethostname (char *, int);
extern int sethostname (char *, int);

#ifdef _sys_socket_h
extern int getpeername(int, struct sockaddr *, int *);
extern int send(int, char *, int, int);
extern int sendto(int, char *, int, int, struct sockaddr *, int);
extern int recv(int, char *, int, int);
extern int recvfrom(int, char *, int, int, struct sockaddr *, int *);
#endif

#ifdef _sys_vfs_h
extern int statfs(const char *, struct statfs *);
#endif

#ifdef __sys_types_h /* def of caddr_t */
extern time_t time(time_t *);
extern int ioctl(int, int, caddr_t);
extern int truncate(char *, off_t);
extern int ftruncate(int, off_t);
caddr_t mmap(caddr_t, size_t, int, int, int, off_t);
int munmap(caddr_t, int);
#endif
extern void perror(char *);

extern int openlog(char *, int, int);

extern int syslog(int, char *, ... );
extern int closelog(void);
extern int setlogmask(int);

extern unsigned int alarm(unsigned int);

extern char *crypt(char *, char *);
extern char *_crypt(char *, char *);
extern int setkey(char *);
extern int encrypt(char *, int);

extern char *getenv(char *name);
extern int putenv(char *);
extern void bcopy(char *, char *, int);
extern void bzero(char *, int);
extern int bcmp(char *, char *, int);
extern int tolower(int);
extern int toupper(int);

#if !defined(__memory_h__)
extern void memset(char *, int, int);
#if !defined(__GNUC__)
extern char *memcpy(char *, char *, int);
extern int  memcmp(char *, char *, int);
#endif
#endif

extern int ffs(int);

extern int sigblock(int);
extern int sigsetmask(int);
extern int sigpause(int);

extern int unmount(char *name);
extern int umount(char *name);
int mount(char *type, char *dir, int flags, char *data);


#ifdef __sys_stat_h
extern int fchmod(int fd, mode_t mode);
#endif

#ifdef _mntent_h
extern FILE *setmntent(char *filep, char *type);
extern struct mntent *getmntent(FILE *filep);
extern int addmntent(FILE *filep, struct mntent *mnt);
extern char *hasmntopt(struct mntent *mnt, char *opt);
extern int endmntent(FILE *filep);
#endif

#ifdef __pwd_h
extern struct passwd *getpwent(void);
extern void endpwent(void);
#endif


extern int usleep(unsigned);
#if 0
extern int lseek(int, int, int);
#endif

#ifndef __sys_unistd_h /* GNUCC has another imagination about this */
extern int read(int, char *, int);
extern int write(int, char *, int);
extern int sleep(unsigned int);
extern int getuid(void);
extern int geteuid(void);
#endif
extern int fsync(int);

extern int rename(const char *, const char *);
extern int unlink(const char *);
#if defined(__sys_dirent_h)
extern int scandir(char *, struct dirent ***, int (*)(), int (*)());
#else
#if defined(_sys_dir_h)
extern int scandir(char *, struct direct ***, int (*)(), int (*)());
#endif
#endif

#ifndef __stdlib_h
extern int atoi(char *);
extern int qsort(char *, int, int, int (*)());
#endif

extern char *rindex(const char *, const char);
extern char *index(const char *, const char);

#ifndef __string_h /* GNUCC string.h */
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, int);
extern char *strcat(char *, const char *);
extern char *strdup(const char *);

extern int strcpy(char *, const char *);
extern int strncpy(char *, const char *, int);

extern int strtok(char *, char *);
#endif

extern int strtol(char *, char **, int);

extern int system(char *);
#ifdef RLIMIT_NOFILE
extern int getrlimit(int, struct rlimit *);
extern int setrlimit(int, struct rlimit *);
#endif

extern int chdir(const char *);
#if defined(_sys_time_h) && defined(__sys_types_h)
extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
extern int getitimer(int, struct itimerval *);
extern int setitimer(int, struct itimerval *, struct itimerval *);
#endif
extern char *getpass(char *);
#if !defined(__malloc_h) && !defined(__stdlib_h) /* GNU malloc defs */
extern void *realloc(void *, unsigned);
extern void *calloc(unsigned, unsigned);
extern void *malloc(unsigned);
extern void free(const char *);
#endif

#ifdef SOCK_STREAM
extern int socket(int, int, int);
extern int connect(int, struct sockaddr *, int);
extern int bind(int, struct sockaddr *, int);
extern int getsockname(int, struct sockaddr *, int *);
extern int accept(int, struct sockaddr *, int *);
extern int listen(int, int);
extern int shutdown(int, int);
extern int inet_addr(char *);
extern int getsockopt(int, int, int, char *, int *);
extern int setsockopt(int, int, int, char *, int);

#endif

#ifdef _nettli_tiuser_h
extern int t_open(char *, int, struct t_info *);
extern void t_error(char *);
extern int t_bind(int, struct t_bind *, struct t_bind *);
extern int t_listen(int, struct t_call *);
extern int t_accept(int, int, struct t_call *);
extern int t_rcv(int, char *, unsigned int, int *);
extern int t_snd(int, char *, unsigned int, int);
extern int t_close(int);
extern int t_connect(int, struct t_call *, struct t_call *);
#endif
#ifdef _sys_poll_h
extern int poll(struct pollfd *, unsigned long, int);
#endif

#ifdef _sys_resource_h
int wait3(int *, int, struct rusage *);
#else
int wait3(int *, int, int);
#endif

#ifdef _sys_asynch_h
extern aio_result_t *aiowait(struct timeval *);
extern int aioread(int, char *, int, int, int, aio_result_t *);
extern int aiowrite(int, char *, int, int, int, aio_result_t *);
extern int aiocancel(aio_result_t *);
#endif

extern int on_exit(void (*procp)(), void *arg);

#endif
