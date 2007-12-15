/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2007 Reuben Thomas <rrt@sc3d.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

// FIXME: Map errors sensibly from EPOC to UNIX

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

#include "plpfuse.h"
#include "rfsv_api.h"

#define NO_PSION	ENOMEDIUM

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
attr2pattr(long oattr, long nattr, long *psisattr, long *psidattr)
{
  /*
   * Following flags have to be set in order to let backups
   * work properly
   */
  *psisattr = *psidattr = 0;
  if ((oattr & 0400) != (nattr & 0400)) {
    if (nattr & 0400)		/* readable */
      *psisattr |= PSI_A_READ;
    else
      *psidattr |= PSI_A_READ;
  }
  if ((oattr & 0200) != (nattr & 0200)) {
    if (nattr & 0200)		/* Not writable   -> readonly */
      *psidattr |= PSI_A_RDONLY;
    else
      *psisattr |= PSI_A_RDONLY;
  }
  if ((oattr & 0020) != (nattr & 0020)) {
    if (nattr & 0020)		/* group-write    -> archive */
      *psisattr |= PSI_A_ARCHIVE;
    else
      *psidattr |= PSI_A_ARCHIVE;
  }
  if ((oattr & 0004) != (nattr & 0004)) {
    if (nattr & 0004)		/* Not world-read -> hidden  */
      *psidattr |= PSI_A_HIDDEN;
    else
      *psisattr |= PSI_A_HIDDEN;
  }
  if ((oattr & 0002) != (nattr & 0002)) {
    if (nattr & 0002)		/* world-write    -> system */
      *psisattr |= PSI_A_SYSTEM;
    else
      *psidattr |= PSI_A_SYSTEM;
  }
}

static void
pattr2attr(long psiattr, long size, long ftime, struct stat *st)
{
  struct fuse_context *ct = fuse_get_context();

  memset(st, 0, sizeof(*st));

  st->st_uid = ct->uid;
  st->st_gid = ct->gid;

  if (psiattr & PSI_A_DIR) {
    st->st_mode = 0700 | S_IFDIR;
    st->st_blocks = 1;
    st->st_size = BLOCKSIZE;
    st->st_nlink = 2; /* Call dircount for more accurate count */
  } else {
    st->st_blocks = (size + BLOCKSIZE - 1) / BLOCKSIZE;
    st->st_size = size;
    st->st_nlink = 1;
    st->st_mode = S_IFREG;

    /*
     * Following flags have to be set in order to let backups
     * work properly
     */
    if (psiattr & PSI_A_READ)
      st->st_mode |= 0400;	/* File readable (?) */
    if (!(psiattr & PSI_A_RDONLY))
      st->st_mode |= 0200;	/* File writeable  */
    /* st->st_mode |= 0100;		   File executable */
    if (!(psiattr & PSI_A_HIDDEN))
      st->st_mode |= 0004;	/* Not Hidden  <-> world read */
    if (psiattr & PSI_A_SYSTEM)
      st->st_mode |= 0002;	/* System      <-> world write */
    if (psiattr & PSI_A_VOLUME)
      st->st_mode |= 0001;	/* Volume      <-> world exec */
    if (psiattr & PSI_A_ARCHIVE)
      st->st_mode |= 0020;	/* Modified    <-> group write */
    /* st->st_mode |= 0040;	 Byte        <-> group read */
    /* st->st_mode |= 0010;	 Text        <-> group exec */
  }

  st->st_mtime = st->st_ctime = st->st_atime = ftime;
}

static device *devices;

static int
query_devices()
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

char *
dirname(const char *dir)
{
  static char *namebuf = NULL;
  if (namebuf)
    free(namebuf);
  asprintf(&namebuf, "%s\\", dir);
  return namebuf;
}

const char *
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
    pattr2attr(e->attr, e->size, e->time, &st);
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
  debuglog("plp_getattr `%s'", ++path);

  if (strcmp(path, "") == 0) {
    pattr2attr(PSI_A_DIR, 0, 0, st);
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
        pattr2attr(PSI_A_DIR, 0, 0, st);
        return getlinks(path, st);
      } else
        return rfsv_isalive() ? -ENOENT : -NO_PSION;
    }

    debuglog("getattr: fileordir");
    if (rfsv_getattr(path, &pattr, &psize, &ptime))
      return rfsv_isalive() ? -ENOENT : -NO_PSION;
    else {
      pattr2attr(pattr, psize, ptime, st);
      debuglog(" attrs Psion: %x %d %d, UNIX modes: %o", pattr, psize, ptime, st->st_mode);
      if (st->st_nlink > 1)
        return getlinks(path, st);
    }
  }

  debuglog("getattr: return OK");
  return 0;
}

static int plp_access(const char *path, int mask)
{
  debuglog("plp_access `%s'", ++path);
  return 0;
}

static int plp_readlink(const char *path, char *buf, size_t size)
{
  debuglog("plp_readlink `%s'", ++path);
  return -EINVAL;
}


static int plp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
  device *dp;
  int ret;
  dentry *e = NULL;

  debuglog("plp_readdir `%s'", ++path);

  (void)offset;
  (void)fi;

  if (strcmp(path, "") == 0) {
    debuglog("readdir root");
    if (query_devices() == 0) {
      for (dp = devices; dp; dp = dp->next) {
        dentry *o;
        struct stat st;
        unsigned char name[3];

        name[0] = dp->letter;
        name[1] = ':';
        name[2] = '\0';
        pattr2attr(dp->attrib, 1, 0, &st);
        if (filler(buf, name, &st, 0))
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

      pattr2attr(e->attr, e->size, e->time, &st);
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

  debuglog("plp_chmod `%s'", ++path);

  if (rfsv_getattr(path, &pattr, &psize, &ptime))
    return rfsv_isalive() ? -ENOENT : -NO_PSION;
  pattr2attr(pattr, psize, ptime, &st);
  attr2pattr(st.st_mode, mode, &psisattr, &psidattr);
  debuglog("  UNIX old, new: %o, %o; Psion set, clear: %x, %x", st.st_mode, mode, psisattr, psidattr);
  if (rfsv_setattr(path, psisattr, psidattr))
    return rfsv_isalive() ? -EACCES : -NO_PSION;

  debuglog("chmod succeeded");
  return 0;
}

static int plp_chown(const char *path, uid_t uid, gid_t gid)
{
  debuglog("plp_chown `%s'", ++path);
  return -EPERM;
}

static int plp_truncate(const char *path, off_t size)
{
  debuglog("plp_truncate `%s'", ++path);

  if (rfsv_setsize(path, 0))
    return rfsv_isalive() ? -EPERM : -NO_PSION;

  return 0;
}

static int plp_utimens(const char *path, const struct timespec ts[2])
{
  struct timeval tv[2];

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
  .chown	= plp_chown,
  .truncate	= plp_truncate,
  .utimens	= plp_utimens,
  .open		= plp_open,
  .read		= plp_read,
  .write	= plp_write,
  .statfs	= plp_statfs,
};
