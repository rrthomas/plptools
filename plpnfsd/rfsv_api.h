/* $Id$/
 *
 * C API for rfsv
 *
 */
#ifndef _rfsv_api_h_
#define _rfsv_api_h_

#include "mp.h"

extern long rfsv_dir(const char *name, dentry **e);
extern long rfsv_mkdir(const char *name);
extern long rfsv_rmdir(const char *name);
extern long rfsv_remove(const char *name);
extern long rfsv_rename(const char *oldname, const char *newname);
extern long rfsv_fclose(long handle);
extern long rfsv_fopen(long attr, const char *name, long *handle);
extern long rfsv_fcreate(long attr, const char *name, long *handle);
extern long rfsv_read(char *buf, long offset, long len, long handle);
extern long rfsv_write(char *buf, long offset, long len, long handle);
extern long rfsv_getattr(const char *name, long *attr, long *size, long *time);
extern long rfsv_setattr(const char *name, long sattr, long dattr);
extern long rfsv_setsize(const char *name, long size);
extern long rfsv_setmtime(const char *name, long time);
extern long rfsv_drivelist(int *cnt, device **devlist);
extern long rfsv_dircount(const char *name, long *count);
extern long rfsv_statdev(char letter);
extern long rfsv_isalive();

#endif
