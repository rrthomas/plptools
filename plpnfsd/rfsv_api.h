/* $Id$/
 *
 * C API for rfsv
 *
 * Copyright (C) 1999 Fritz Elfert <felfert@to.com>
 *
 */
#ifndef _rfsv_api_h_
#define _rfsv_api_h_

#include "mp.h"
#include "builtins.h"

extern long rfsv_dir(const char *name, dentry **e);
extern long rfsv_mkdir(const char *name);
extern long rfsv_rmdir(const char *name);
extern long rfsv_remove(const char *name);
extern long rfsv_rename(const char *oldname, const char *newname);
extern long rfsv_fclose(long handle);
extern long rfsv_fcreate(long attr, const char *name, long *handle);
extern long rfsv_read(char *buf, long offset, long len, char *name);
extern long rfsv_write(char *buf, long offset, long len, char *name);
extern long rfsv_getattr(const char *name, long *attr, long *size, long *time);
extern long rfsv_setattr(const char *name, long sattr, long dattr);
extern long rfsv_setsize(const char *name, long size);
extern long rfsv_setmtime(const char *name, long time);
extern long rfsv_drivelist(int *cnt, device **devlist);
extern long rfsv_dircount(const char *name, long *count);
extern long rfsv_statdev(char letter);
extern long rfsv_isalive();
extern long rfsv_closecached(void);

extern long rpcs_ownerRead(builtin_node *, char *buf, unsigned long  offset, long len);
extern long rpcs_ownerSize(builtin_node *);

/* File attributes, C-style */
#define	PSI_A_RDONLY		0x0001
#define	PSI_A_HIDDEN		0x0002
#define	PSI_A_SYSTEM		0x0004
#define PSI_A_DIR		0x0008
#define PSI_A_ARCHIVE		0x0010
#define PSI_A_VOLUME		0x0020
#define PSI_A_NORMAL		0x0040
#define PSI_A_TEMP		0x0080
#define PSI_A_COMPRESSED	0x0100
#define PSI_A_READ		0x0200
#define PSI_A_EXEC		0x0400
#define PSI_A_STREAM		0x0800
#define PSI_A_TEXT		0x1000

#endif
