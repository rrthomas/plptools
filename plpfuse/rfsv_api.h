/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef _rfsv_api_h_
#define _rfsv_api_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "plpfuse.h"

extern long rfsv_dir(const char *name, dentry **e);
extern long rfsv_mkdir(const char *name);
extern long rfsv_rmdir(const char *name);
extern long rfsv_remove(const char *name);
extern long rfsv_rename(const char *oldname, const char *newname);
extern long rfsv_open(const char *name, long mode, u_int32_t *handle);
extern long rfsv_fclose(long handle);
extern long rfsv_fcreate(long attr, const char *name, u_int32_t *handle);
extern long rfsv_read(char *buf, long offset, long len, const char *name);
extern long rfsv_write(const char *buf, long offset, long len, const char *name);
extern long rfsv_getattr(const char *name, long *attr, long *size, long *time);
extern long rfsv_setattr(const char *name, long sattr, long dattr);
extern long rfsv_setsize(const char *name, long size);
extern long rfsv_setmtime(const char *name, long time);
extern long rfsv_drivelist(int *cnt, device **devlist);
extern long rfsv_dircount(const char *name, long *count);
extern long rfsv_statdev(char letter);
extern long rfsv_isalive();

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

#ifdef __cplusplus
}
#endif

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
