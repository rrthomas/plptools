/* $Id$
 *
 */

#ifndef _plpfuse_h_
#define _plpfuse_h_

#include <fuse.h>

typedef struct p_inode {
	int inode;
	char *name;
	struct p_inode *nextnam, *nextnum;
} p_inode;

/**
 * Description of a Psion-Device
 */
typedef struct p_device {
  char *name;  /* Volume-Name */
  char letter; /* Drive-Letter */
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

extern int debug;

extern void debuglog(char *fmt, ...);

#define BLOCKSIZE      512
#define FID            7 /* File system id */

#endif

extern struct fuse_operations plp_oper;
