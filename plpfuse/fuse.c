/*
  plpfuse: Expose EPOC's file system via FUSE
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2007-2008  Reuben Thomas <rrt@sc3d.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/* FIXME: Map errors sensibly from EPOC to UNIX */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>
#include <attr/xattr.h>
#include <attr/attributes.h>

#include "plpfuse.h"
#include "rfsv_api.h"

#define NO_PSION	ENOMEDIUM

/* Name of our extended attribute */
#define XATTR_NAME "user.psion"

/* Maximum length of a generated psion xattr string */
#define XATTR_MAXLEN 3

int debug;

int
debuglog(char *fmt, ...)
{
  va_list ap;
  char *buf;

  if (!debug)
    return 0;
  va_start(ap, fmt);
  vasprintf(&buf, fmt, ap);
  syslog(LOG_DEBUG, "%s", buf);
  free(buf);
  va_end(ap);
  return 0;
}

static void
xattr2pattr(long *psisattr, long *psidattr, const char *oxattr, const char *nxattr)
{
  if ((strchr(oxattr, 'a') == NULL) != (strchr(nxattr, 'a') == NULL)) { /* a = archive */
    if (strchr(nxattr, 'a'))
      *psisattr |= PSI_A_ARCHIVE;
    else
      *psidattr |= PSI_A_ARCHIVE;
  }
  if ((strchr(oxattr, 'h') == NULL) != (strchr(nxattr, 'h') == NULL)) { /* h = hidden */
    if (strchr(nxattr, 'h'))
      *psidattr |= PSI_A_HIDDEN;
    else
      *psisattr |= PSI_A_HIDDEN;
  }
  if ((strchr(oxattr, 's') == NULL) != (strchr(nxattr, 's') == NULL)) { /* s = system */
    if (strchr(nxattr, 's'))
      *psisattr |= PSI_A_SYSTEM;
    else
      *psidattr |= PSI_A_SYSTEM;
  }
}

static void
attr2pattr(long oattr, long nattr, char *oxattr, char *nxattr, long *psisattr, long *psidattr)
{
  *psisattr = *psidattr = 0;
  if ((oattr & 0400) != (nattr & 0400)) {
    if (nattr & 0400) /* readable */
      *psisattr |= PSI_A_READ;
    else
      *psidattr |= PSI_A_READ;
  }
  if ((oattr & 0200) != (nattr & 0200)) {
    if (nattr & 0200) /* Not writable   -> readonly */
      *psidattr |= PSI_A_RDONLY;
    else
      *psisattr |= PSI_A_RDONLY;
  }
  xattr2pattr(psisattr, psidattr, oxattr, nxattr);
}

static void
pattr2xattr(long psiattr, char *xattr)
{
  *xattr = '\0';

  if (psiattr & PSI_A_HIDDEN)
    strcat(xattr, "h");
  if (psiattr & PSI_A_SYSTEM)
    strcat(xattr, "s");
  if (psiattr & PSI_A_ARCHIVE)
    strcat(xattr, "a");
}

static void
pattr2attr(long psiattr, long size, long ftime, struct stat *st, char *xattr)
{
  struct fuse_context *ct = fuse_get_context();

  memset(st, 0, sizeof(*st));
  st->st_uid = ct->uid;
  st->st_gid = ct->gid;

  if (psiattr & PSI_A_DIR) {
    st->st_mode = 0700 | S_IFDIR;
    st->st_blocks = 1;
    st->st_size = BLOCKSIZE;
    st->st_nlink = 2; /* Call getlinks for more accurate count */
  } else {
    st->st_blocks = (size + BLOCKSIZE - 1) / BLOCKSIZE;
    st->st_size = size;
    st->st_nlink = 1;
    st->st_mode = S_IFREG;

    if (psiattr & PSI_A_READ)
      st->st_mode |= 0400;	/* File readable */
    if (!(psiattr & PSI_A_RDONLY))
      st->st_mode |= 0200;	/* File writeable  */
  }
  st->st_mtime = st->st_ctime = st->st_atime = ftime;
  pattr2xattr(psiattr, xattr);
}

static device *devices;

static int
query_devices(void)
{
  device *dp, *np;
  int link_count = 2;	/* set the root link count */

  for (dp = devices; dp; dp = np) {
    np = dp->next;
    free(dp->name);
    free(dp);
  }
  devices = NULL;
  if (rfsv_drivelist(&link_count, &devices))
    return 1;
  return 0;
}

static char *
dirname(const char *dir)
{
  static char *namebuf = NULL;
  if (namebuf)
    free(namebuf);
  asprintf(&namebuf, "%s\\", dir);
  return namebuf;
}

static const char *
filname(const char *dir)
{
  char *p;
  if ((p = (char *) rindex(dir, '\\')))
    return p + 1;
  else
    return dir;
}

static int
dircount(const char *path, long *count)
{
  dentry *e = NULL;
  long ret = 0;

  *count = 0;
  debuglog("dircount: %s", path);
  debuglog("RFSV dir %s", path);
  if ((ret = rfsv_dir(dirname(path), &e)))
    return ret;
  while (e) {
    struct stat st;
    dentry *o = e;
    char xattr[XATTR_MAXLEN + 1];
    pattr2attr(e->attr, e->size, e->time, &st, xattr);
    free(e->name);
    e = e->next;
    free(o);
    if (st.st_nlink > 1)
      (*count)++;
  }

  debuglog("count %d", *count);
  return ret;
}

static int getlinks(const char *path, struct stat *st)
{
  long dcount;

  if (dircount(path, &dcount))
    return rfsv_isalive() ? -ENOENT : -NO_PSION;
  st->st_nlink = dcount + 2;
  return 0;
}

static int plp_getattr(const char *path, struct stat *st)
{
  char xattr[XATTR_MAXLEN + 1];

  debuglog("plp_getattr `%s'", ++path);

  if (strcmp(path, "") == 0) {
    pattr2attr(PSI_A_DIR, 0, 0, st, xattr);
    if (!query_devices()) {
      device *dp;
                
      for (dp = devices; dp; dp = dp->next)
        st->st_nlink++;
      debuglog("root has %d links", st->st_nlink);
    } else
      return rfsv_isalive() ? -ENOENT : -NO_PSION;
  } else {
    long pattr, psize, ptime;

    if (strlen(path) == 2 && path[1] == ':') {
      debuglog("getattr: device");
      if (!query_devices()) {
        device *dp;
                
        for (dp = devices; dp; dp = dp->next) {
          debuglog("cmp '%c', '%c'", dp->letter,
                   path[0]);
          if (dp->letter == path[0])
            break;
        }
        debuglog("device: %s", dp ? "exists" : "does not exist");
        pattr2attr(PSI_A_DIR, 0, 0, st, xattr);
        return getlinks(path, st);
      } else
        return rfsv_isalive() ? -ENOENT : -NO_PSION;
    }

    debuglog("getattr: fileordir");
    if (rfsv_getattr(path, &pattr, &psize, &ptime))
      return rfsv_isalive() ? -ENOENT : -NO_PSION;
    else {
      pattr2attr(pattr, psize, ptime, st, xattr);
      debuglog(" attrs Psion: %x %d %d, UNIX modes: %o, xattrs: %s", pattr, psize, ptime, st->st_mode, xattr);
      if (st->st_nlink > 1)
        return getlinks(path, st);
    }
  }

  debuglog("getattr: return OK");
  return 0;
}

static int plp_access(const char *path, int mask)
{
  (void)mask;
  debuglog("plp_access `%s'", ++path);
  return 0;
}

static int plp_readlink(const char *path, char *buf, size_t size)
{
  (void)buf;
  (void)size;
  debuglog("plp_readlink `%s'", ++path);
  return -EINVAL;
}


static int plp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
  device *dp;
  dentry *e = NULL;
  char xattr[XATTR_MAXLEN + 1];

  debuglog("plp_readdir `%s'", ++path);

  (void)offset;
  (void)fi;

  if (strcmp(path, "") == 0) {
    debuglog("readdir root");
    if (query_devices() == 0) {
      for (dp = devices; dp; dp = dp->next) {
        struct stat st;
        unsigned char name[3];

        name[0] = dp->letter;
        name[1] = ':';
        name[2] = '\0';
        pattr2attr(dp->attrib, 1, 0, &st, xattr);
        if (filler(buf, (char *)name, &st, 0))
          break;
      }
    }
  } else {
    debuglog("RFSV dir `%s'", dirname(path));
    if (rfsv_dir(dirname(path), &e))
      return rfsv_isalive() ? -ENOENT : -NO_PSION;

    debuglog("scanning contents");
    while (e) {
      dentry *o;
      struct stat st;
      const char *name = filname(e->name);

      pattr2attr(e->attr, e->size, e->time, &st, xattr);
      debuglog("  %s %o %d %d", name, st.st_mode, st.st_size, st.st_mtime);
      if (filler(buf, name, &st, 0))
        break;
      free(e->name);
      o = e;
      e = e->next;
      free(o);
    }
  }

  debuglog("readdir OK");
  return 0;
}

static int plp_mknod(const char *path, mode_t mode, dev_t dev)
{
  u_int32_t phandle;

  debuglog("plp_mknod `%s' %o", ++path, mode);

  if (S_ISREG(mode) && dev == 0) {
    if (rfsv_fcreate(0x200, path, &phandle))
      return rfsv_isalive() ? -ENAMETOOLONG : -NO_PSION;
    rfsv_fclose(phandle);
  } else
    return -EINVAL;

  return 0;
}

static int plp_mkdir(const char *path, mode_t mode)
{
  debuglog("plp_mkdir `%s' %o", ++path, mode);

  if (rfsv_mkdir(path))
    return rfsv_isalive() ? -ENAMETOOLONG : -NO_PSION;

  return 0;
}

static int plp_unlink(const char *path)
{
  debuglog("plp_unlink `%s'", ++path);

  if (rfsv_remove(path))
    return rfsv_isalive() ? -EACCES : -NO_PSION;

  return 0;
}

static int plp_rmdir(const char *path)
{
  debuglog("plp_rmdir `%s'", ++path);

  if (rfsv_rmdir(path))
    return rfsv_isalive() ? -EACCES : -NO_PSION;

  return 0;
}

static int plp_symlink(const char *from, const char *to)
{
  debuglog("plp_symlink `%s' -> `'%s'", ++from, ++to);
  return -EPERM;
}

static int plp_rename(const char *from, const char *to)
{
  debuglog("plp_rename `%s' -> `%s'", ++from, ++to);

  rfsv_remove(to);
  if (rfsv_rename(from, to))
    return rfsv_isalive() ? -EACCES : -NO_PSION;

  return 0;
}

static int plp_link(const char *from, const char *to)
{
  debuglog("plp_link `%s' -> `%s'", ++from, ++to);
  return -EPERM;
}

static int plp_chmod(const char *path, mode_t mode)
{
  long psisattr, psidattr, pattr, psize, ptime;
  struct stat st;
  char xattr[XATTR_MAXLEN + 1];

  debuglog("plp_chmod `%s'", ++path);

  if (rfsv_getattr(path, &pattr, &psize, &ptime))
    return rfsv_isalive() ? -ENOENT : -NO_PSION;
  pattr2attr(pattr, psize, ptime, &st, xattr);
  attr2pattr(st.st_mode, mode, "", "", &psisattr, &psidattr);
  debuglog("  UNIX old, new: %o, %o; Psion set, clear: %x, %x", st.st_mode, mode, psisattr, psidattr);
  if (rfsv_setattr(path, psisattr, psidattr))
    return rfsv_isalive() ? -EACCES : -NO_PSION;

  debuglog("chmod succeeded");
  return 0;
}

static int plp_getxattr(const char *path, const char *name, char *value, size_t size)
{
  debuglog("plp_getxattr `%s'", ++path);
  if (strcmp(name, XATTR_NAME) == 0) {
    if (size >= XATTR_MAXLEN) {
      long pattr, psize, ptime;
      if (rfsv_getattr(path, &pattr, &psize, &ptime))
        return rfsv_isalive() ? -ENOENT : -NO_PSION;
      pattr2xattr(pattr, value);
      debuglog("getxattr succeeded: %s", value);
      return strlen(value);
    } else {
      debuglog("only gave %d bytes, need %d", size, XATTR_MAXLEN);
      return XATTR_MAXLEN;
    }
  } else {
    errno = ENOATTR;
    return -1;
  }
}

static int plp_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
  debuglog("plp_setxattr `%s'", ++path);
  if (strcmp(name, XATTR_NAME) == 0) {
    long psisattr, psidattr;
    char oxattr[XATTR_MAXLEN + 1], nxattr[XATTR_MAXLEN + 1];

    if (flags & XATTR_CREATE) {
      errno = EEXIST;
      return -1;
    }

    strncpy(nxattr, value, size < XATTR_MAXLEN ? size : XATTR_MAXLEN);
    nxattr[XATTR_MAXLEN] = '\0';
    /* Need to undo earlier increment of path when calling plp_getxattr */
    plp_getxattr(path - 1, name, oxattr, XATTR_MAXLEN);
    psisattr = psidattr = 0;
    xattr2pattr(&psisattr, &psidattr, oxattr, value);
    debuglog("attrs set %x delete %x; %s, %s", psisattr, psidattr, oxattr, value);
    if (rfsv_setattr(path, psisattr, psidattr))
      return rfsv_isalive() ? -EACCES : -NO_PSION;

    debuglog("setxattr succeeded");
    return 0;
  } else {
    if (flags & XATTR_REPLACE)
      errno = ENOATTR;
    else
      errno = ENOTSUP;
    return -1;
  }
}

static int plp_listxattr(const char *path, char *list, size_t size)
{
  debuglog("plp_listxattr `%s'", ++path);
  if (size > sizeof(XATTR_NAME))
    strcpy(list, XATTR_NAME);
  return sizeof(XATTR_NAME);
}

static int plp_removexattr(const char *path, const char *name)
{
  debuglog("plp_removexattr `%s'", ++path);
  errno = ENOTSUP;
  return -1;
}

static int plp_chown(const char *path, uid_t uid, gid_t gid)
{
  (void)uid;
  (void)gid;
  debuglog("plp_chown `%s'", ++path);
  return -EPERM;
}

static int plp_truncate(const char *path, off_t size)
{
  (void)size;
  debuglog("plp_truncate `%s'", ++path);

  if (rfsv_setsize(path, 0))
    return rfsv_isalive() ? -EPERM : -NO_PSION;

  return 0;
}

static int plp_utimens(const char *path, const struct timespec ts[2])
{
  debuglog("plp_utimens `%s'", ++path);

  if (rfsv_setmtime(path, ts[1].tv_sec))
    return rfsv_isalive() ? -EPERM : -NO_PSION;

  return 0;
}

static int plp_open(const char *path, struct fuse_file_info *fi)
{
  debuglog("plp_open `%s'", ++path);
  (void)fi;
  return 0;
}

static int plp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
  long read;

  (void)fi;
  debuglog("plp_read `%s' offset %lld size %ld", ++path, offset, size);

  if ((read = rfsv_read(buf, (long)offset, size, path)) < 0)
    return rfsv_isalive() ? -ENOENT : -NO_PSION;

  debuglog("read %ld bytes", read);
  return read;
}

static int plp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
  long written;

  (void)fi;
  debuglog("plp_write `%s' offset %lld size %ld", ++path, offset, size);
  if ((written = rfsv_write(buf, offset, size, path)) < 0)
    return rfsv_isalive() ? -ENOSPC : -NO_PSION;

  debuglog("wrote %ld bytes", written);
  return written;
}

static int plp_statfs(const char *path, struct statvfs *stbuf)
{
  device *dp;

  (void)path;
  debuglog("plp_statfs");

  stbuf->f_bsize = BLOCKSIZE;
  stbuf->f_frsize = BLOCKSIZE;
  if (query_devices() == 0) {
    for (dp = devices; dp; dp = dp->next) {
      stbuf->f_blocks += (dp->total + BLOCKSIZE - 1) / BLOCKSIZE;
      stbuf->f_bfree += (dp->free + BLOCKSIZE - 1) / BLOCKSIZE;
    }
  }
  stbuf->f_bavail = stbuf->f_bfree;

  /* Don't have numbers for these */
  stbuf->f_files = 0;
  stbuf->f_ffree = stbuf->f_favail = 0;

  stbuf->f_fsid = FID;
  stbuf->f_flag = 0;    /* don't have mount flags */
  stbuf->f_namemax = 255; /* KDMaxFileNameLen% */
    
  return 0;
}

struct fuse_operations plp_oper = {
  .getattr	= plp_getattr,
  .access	= plp_access,
  .readlink	= plp_readlink,
  .readdir	= plp_readdir,
  .mknod	= plp_mknod,
  .mkdir	= plp_mkdir,
  .symlink	= plp_symlink,
  .unlink	= plp_unlink,
  .rmdir	= plp_rmdir,
  .rename	= plp_rename,
  .link		= plp_link,
  .chmod	= plp_chmod,
  .setxattr	= plp_setxattr,
  .getxattr	= plp_getxattr,
  .listxattr	= plp_listxattr,
  .removexattr	= plp_removexattr,
  .chown	= plp_chown,
  .truncate	= plp_truncate,
  .utimens	= plp_utimens,
  .open		= plp_open,
  .read		= plp_read,
  .write	= plp_write,
  .statfs	= plp_statfs,
};
