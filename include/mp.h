/* $Id$
 *
 */

#ifndef _mp_h_
#define _mp_h_

#include "fparam.h"
#include "nfs_prot.h"

#define PFS_OP_CREATE   0x01
#define PFS_OP_GETATTR  0x02
#define PFS_OP_MKDIR    0x03
#define PFS_OP_READ     0x04
#define PFS_OP_READDIR  0x05
#define PFS_OP_REMOVE   0x06
#define PFS_OP_RENAME   0x07
#define PFS_OP_RMDIR    0x08
#define PFS_OP_SETATTR  0x09
#define PFS_OP_WRITE    0x0a
#define PFS_OP_GETDEVS  0x0b
/* The same as getattr without the stat (for linkcount purposes */
#define PFS_OP_STATDEV  0x0c

/* Only for the new nfsc */
#define PFS_OP_TTYDATA  0x0d
#define PFS_OP_EXEC  	0x0e
#define PFS_OP_ECHO  	0x0f
#define PREFIX	0x80

typedef struct p_inode
{
  int inode;
  char *name;
  struct p_inode *nextnam, *nextnum;
} p_inode;

/*
 * Description of a Psion-Device
 */
typedef struct p_device
{
  char *name;  /* Volume-Name */
  char letter; /* Drive-Number (zero-based, i.e. 0 = A, 1 = B, 2 = C ...) */
  long attrib; /* Device-Attribs */
  long total;  /* Total capacity in bytes */
  long free;   /* Free space in bytes */
  struct p_device *next;
} device;

/*
 * Description of a Psion-File/Dir
 */
typedef struct p_dentry
{
	char *name;
	long time;
	long attr;
	long size;
	long links;
	struct p_dentry *next;
} dentry;

/* data cache */
struct dcache
{
  struct dcache *next;
  unsigned char *data;
  unsigned int offset, len;
  int towrite;
};

/* attribute cache */
struct cache
{
  struct cache *next;
  unsigned int inode;
  int actual_size;
  struct dcache *dcache;
  fattr attr;
};

struct entrycache
{
  int inode;
  entry *head;
  struct entrycache *next;
};

extern int debug, gmtoffset, exiting, psion_alive, dowakeup,
           query_cache, series5;
extern fattr root_fattr;
extern struct cache *datacache, *attrcache;

#ifdef __SVR4
#define bzero(a,b) memset(a,0,b)
#define bcopy(a,b,c) memcpy(b,a,c)
#define bcmp(a,b,n) memcmp(a,b,n)
#define index strchr
#define rindex strrchr
#endif

#if !defined(__STDC__)
extern char *index(), *rindex(), *strdup();
#endif


#define THE_END        (struct entry *)-1
#define PBUFSIZE       8192

#define TIMEOUT       -54
#define BLOCKSIZE      512
#define FID            7 /* File system id */

#if defined(sun) && defined(__SVR4)
/* 
 * at least /opt/SUNWspro/bin/cc on Solaris 2.4 likes these:
 */
# define SIGARG (int arg)
#else
# define SIGARG ()
#endif

/* mp_main.c */
#if defined(hpux) || defined(__SVR4)
	/* HPUX 10.20 declares int usleep( useconds_t useconds); */
#  ifndef HPUX10
     extern void usleep __P((int usec));
#  endif
#endif

/* mp_serial.c */
int init_serial __P((char *dev, int speed));
int fd_is_still_alive __P((int fd, int wake));
void reset_serial __P((int fd));

/* crc.c */
int docrc16 __P((unsigned char *, int));

/* mp_mount.c */
void mount_and_run __P((char *dir, void (*proc)(), nfs_fh *root_fh));

/* mp_inode.c */
extern p_inode *get_num __P((int));
extern p_inode *get_nam __P((char *));
extern p_inode *re_nam __P((char *, char *));
extern void inode2fh __P((int, char *));

extern char *dirname __P((char *));
extern char *filname __P((char *));
extern char *build_path __P((char *, char *));

extern int fh2inode __P((char *));
extern int getpinode __P((p_inode *inode));
extern char *iso2cp __P((char *));
extern char *cp2iso __P((char *));

extern struct cache *add_cache __P((struct cache **, unsigned int, fattr *));
extern void          rem_cache __P((struct cache **, unsigned int));
extern void          clean_cache __P((struct cache **));
extern struct cache *search_cache __P((struct cache *, unsigned int));

extern struct dcache *add_dcache __P((struct cache *, unsigned int, unsigned int, unsigned char *));
extern void           clean_dcache __P((struct cache *));
extern struct dcache *search_dcache __P((struct cache *, unsigned int, unsigned int));

/* mp_xmit.c */
int sendop __P((int cmd, char *fname, char *rest, int restlen));
int getstr __P((char *str));
int getanswer __P((void));
int sendcmd __P((int cmd, char *fname, char *rest, int restlen));
int getcount __P((unsigned char *str, int num));
int senddata __P((char *p, int len));
void long2pstr __P((unsigned int l, unsigned char *s));
void short2pstr __P((unsigned int l, unsigned char *s));
unsigned int pstr2long __P((unsigned char *s));

/* mp_pfs_ops.c */
extern void *nfsproc_null_2 __P((void));
extern void *nfsproc_root_2 __P((void));
extern void *nfsproc_writecache_2 __P((void));
extern nfsstat *nfsproc_link_2 __P((struct linkargs *la));
extern nfsstat *nfsproc_rmdir_2 __P((struct diropargs *da));
extern nfsstat *nfsproc_remove_2 __P((struct diropargs *da));
extern nfsstat *nfsproc_rename_2 __P((struct renameargs *ra));
extern nfsstat *nfsproc_symlink_2 __P((struct symlinkargs *sa));
extern struct readres *nfsproc_read_2 __P((struct readargs *ra));
extern struct attrstat *nfsproc_write_2 __P((struct writeargs *wa));
extern struct diropres *nfsproc_mkdir_2 __P((struct createargs *ca));
extern struct diropres *nfsproc_create_2 __P((struct createargs *ca));
extern struct diropres *nfsproc_lookup_2 __P((struct diropargs *da));
extern struct attrstat *nfsproc_getattr_2 __P((struct nfs_fh *fh));
extern struct attrstat *nfsproc_setattr_2 __P((struct sattrargs *sa));
extern struct statfsres *nfsproc_statfs_2 __P((struct nfs_fh *fh));
extern struct readdirres *nfsproc_readdir_2 __P((struct readdirargs *ra));
extern struct readlinkres *nfsproc_readlink_2 __P((struct nfs_fh *fh));

extern int mp_main(int, char *, char *);

extern int logsyslog;
extern int debuglog(char *fmt, ...);
extern int errorlog(char *fmt, ...);
extern int infolog(char *fmt, ...);

#endif
