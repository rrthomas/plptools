#include "OSdefs.h"
#include <stdio.h>
#include <ctype.h>
#if defined(__SVR4) || defined(__GLIBC__)
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif
#include "nfs_prot.h"
#include "mp.h"
#include "rfsv_api.h"

static device *devices;
struct cache *attrcache;

/*
 * Nfsd returned NFSERR_STALE if the psion wasn't present, but I didn't like
 * it because the kernel returns the same when the nfsd itself is absent
 */

#define NO_PSION	NFSERR_NXIO

/* FIXME: Send create attributes */
static struct diropres *
create_it(createargs *ca, int isdir)
{
	static struct diropres res;
	p_inode *dirinode = get_num(fh2inode(ca->where.dir.data));
	char *name = dirinode->name;
	fattr *fp;
	p_inode *inode;
	long phandle;
	int rfsv_ret;

	if (debug)
		printf("\tcreate: in %s %s (%#o, %d)\n",
		       name, ca->where.name, ca->attributes.mode, isdir);

	name = build_path(name, ca->where.name);

	if (isdir)
		rfsv_ret = rfsv_mkdir(name);
	else {
		rfsv_ret = rfsv_fcreate(0x200, name, &phandle);
		if (rfsv_ret == 0)
			rfsv_ret = rfsv_fclose(phandle);
	}
	if (rfsv_ret) {
		res.status = psion_alive ? NFSERR_NAMETOOLONG : NO_PSION;
		return &res;
	}
	inode = get_nam(name);
	inode2fh(inode->inode, res.diropres_u.diropres.file.data);

	fp = &res.diropres_u.diropres.attributes;
	bzero((char *) fp, sizeof(fp));
	if (isdir) {
		fp->type = NFDIR;
		fp->mode = NFSMODE_DIR | 0700;
		fp->nlink = 2;
	} else {
		fp->type = NFREG;
		fp->mode = NFSMODE_REG | 0600;
		fp->nlink = 1;
	}

	fp->uid = root_fattr.uid;
	fp->gid = root_fattr.gid;
	fp->blocksize = BLOCKSIZE;
	fp->fileid = inode->inode;
	fp->atime.seconds = fp->mtime.seconds = fp->ctime.seconds = time((time_t *) 0);

	res.status = NFS_OK;

	rem_cache(&attrcache, dirinode->inode);
	rem_cache(&attrcache, inode->inode);
	add_cache(&attrcache, inode->inode, fp);
	return &res;
}

struct diropres *
nfsproc_create_2(createargs *ca)
{
	return create_it(ca, 0);
}

struct diropres *
nfsproc_mkdir_2(createargs *ca)
{
	return create_it(ca, 1);
}

static void
attr2pattr(long oattr, long nattr, long *psisattr, long *psidattr)
{
	/*
	 * Following flags have to be set in order to let backups
	 * work properly
	 */
	*psisattr = *psidattr = 0;
	if ((oattr & 0200) != (nattr & 0200)) {
		if (nattr & 0200)		/* readonly */
			*psidattr |= 0x01;
		else
			*psisattr |= 0x01;
	}
	if ((oattr & 0020) != (nattr & 0020)) {
		if (nattr & 0020)	/* group-write    -> archive */
			*psisattr |= 0x20;
		else
			*psidattr |= 0x20;
	}
	if ((oattr & 0004) != (nattr & 0004)) {
		if (nattr & 0004)		/* Not world-read -> hidden  */
			*psidattr |= 0x02;
		else
			*psisattr |= 0x02;
	}
	if ((oattr & 0002) != (nattr & 0002)) {
		if (nattr & 0002)		/* world-write    -> system */
			*psisattr |= 0x04;
		else
			*psidattr |= 0x04;
	}
}

static void
dpattr2attr(long psiattr, long size, long ftime, fattr *fp, int inode)
{
	bzero((char *) fp, sizeof(*fp));

	if (psiattr & 0x10) {
		fp->type = NFDIR;
		fp->mode = NFSMODE_DIR | 0700;
		/*
		 * Damned filesystem.
		 * We have to count the number of subdirectories
		 * on the psion.
		 */
		fp->nlink = 0;
		fp->size = BLOCKSIZE;
		fp->blocks = 1;
	} else {
		fp->type = NFREG;
		fp->mode = NFSMODE_REG;
		fp->nlink = 1;
		fp->size = size;
		fp->blocks = (fp->size + BLOCKSIZE - 1) / BLOCKSIZE;

		/*
		 * Following flags have to be set in order to let backups
		 * work properly
		 */
		fp->mode |= 0400;		/* File readable (?) */
		if (!(psiattr & 0x01))
			fp->mode |= 0200;	/* File writeable  */
		/* fp->mode |= 0100;		   File executable */
		if (!(psiattr & 0x02))
			fp->mode |= 0004;	/* Not Hidden  <-> world read */
		if (psiattr & 0x04)
			fp->mode |= 0002;	/* System      <-> world write */
		if (psiattr & 0x40)
			fp->mode |= 0001;	/* Volume      <-> world exec */
		if (psiattr & 0x20)
			fp->mode |= 0020;	/* Modified    <-> group write */
	/*		fp->mode |= 0040;	 Byte        <-> group read */
	/*		fp->mode |= 0010;	 Text        <-> group exec */
	}

	fp->uid = root_fattr.uid;
	fp->gid = root_fattr.gid;
	fp->blocksize = BLOCKSIZE;
	fp->fileid = inode;
	fp->rdev = fp->fsid = FID;
	fp->atime.seconds = ftime;
	fp->mtime.seconds = fp->ctime.seconds = fp->atime.seconds;
}

static void
pattr2attr(long psiattr, long size, long ftime, fattr *fp, unsigned char *fh)
{
	bzero((char *) fp, sizeof(*fp));

	if (psiattr & 0x10) {
		fp->type = NFDIR;
		fp->mode = NFSMODE_DIR | 0700;
		/*
		 * Damned filesystem.
		 * We have to count the number of subdirectories
		 * on the psion.
		 */
		fp->nlink = 0;
		fp->size = BLOCKSIZE;
		fp->blocks = 1;
	} else {
		fp->type = NFREG;
		fp->mode = NFSMODE_REG;
		fp->nlink = 1;
		fp->size = size;
		fp->blocks = (fp->size + BLOCKSIZE - 1) / BLOCKSIZE;

		/*
		 * Following flags have to be set in order to let backups
		 * work properly
		 */
		fp->mode |= 0400;		/* File readable (?) */
		if (!(psiattr & 0x01))
			fp->mode |= 0200;	/* File writeable  */
		/* fp->mode |= 0100;		   File executable */
		if (!(psiattr & 0x02))
			fp->mode |= 0004;	/* Not Hidden  <-> world read */
		if (psiattr & 0x04)
			fp->mode |= 0002;	/* System      <-> world write */
		if (psiattr & 0x40)
			fp->mode |= 0001;	/* Volume      <-> world exec */
		if (psiattr & 0x20)
			fp->mode |= 0020;	/* Modified    <-> group write */
	/*		fp->mode |= 0040;	 Byte        <-> group read */
	/*		fp->mode |= 0010;	 Text        <-> group exec */
	}

	fp->uid = root_fattr.uid;
	fp->gid = root_fattr.gid;
	fp->blocksize = BLOCKSIZE;
	fp->fileid = fh2inode((char *) fh);
	fp->rdev = fp->fsid = FID;
	fp->atime.seconds = ftime;
	fp->mtime.seconds = fp->ctime.seconds = fp->atime.seconds;
}

static int
query_devices()
{
	device *dp, *np;
	int link_count = 2;	/* set the root link count */

	if (query_cache)
		return 0;
	query_cache = 1;
	for (dp = devices; dp; dp = np) {
		np = dp->next;
		free(dp->name);
		free(dp);
	}
	devices = 0;
	if (rfsv_drivelist(&link_count, &devices))
		return 1;
	root_fattr.nlink = link_count;
	return 0;
}

static int
mp_dircount(p_inode *inode, long *count)
{
	dentry *e = NULL;
	long ret;

	*count = 0;
	if (debug)
		printf("\tdircount: dir\n");
	if ((ret = rfsv_dir(dirname(inode->name), &e)))
		return ret;
	while (e) {
		fattr fp;
		char *bp;
		dentry *o;
		int ni;

		bp = filname(e->name);
		ni = get_nam(build_path(inode->name, bp))->inode;
		free(e->name);
		if (!search_cache(attrcache, ni)) {
			dpattr2attr(e->attr, e->size, e->time, &fp, ni);
			add_cache(&attrcache, ni, &fp);
		}
		o = e;
		e = e->next;
		free(o);
		(*count)++;
	}
	return 0;
}

struct attrstat *
nfsproc_getattr_2(struct nfs_fh *fh)
{
	static struct attrstat res;
	p_inode *inode = get_num(fh2inode(fh->data));
	fattr *fp = &res.attrstat_u.attributes;
	struct cache *cp;
	long pattr;
	long psize;
	long ptime;
	long dcount;
	int l;

	if (debug)
		printf("\tgetattr:'%s',%d\n", inode->name, inode->inode);
	res.status = NFS_OK;

	if ((cp = search_cache(attrcache, inode->inode))) {
		if (debug)
			printf("\t\tgetattr: cache hit\n");
		*fp = cp->attr;	/* gotcha */
		if (fp->type == NFDIR) {
			if (mp_dircount(inode, &dcount)) {
				res.status = psion_alive ? NFSERR_NOENT : NO_PSION;
				return &res;
			}
			if (fp->nlink != (dcount + 2))
				fp->mtime.seconds = time(0);
			fp->nlink = dcount + 2;
			cp->attr = *fp;
		}
		return &res;
	}
	l = strlen(inode->name);

	if (inode->inode == root_fattr.fileid) {
		/* It's the root inode */
		if (debug)
			printf("\t\tgetattr:root inode (%#o)\n", root_fattr.mode);

		if (query_devices())	/* root inode is always there */
			root_fattr.nlink = 2;
		*fp = root_fattr;
	} else if (l == 2 && inode->name[1] == ':') {
		if (debug)
			printf("\tgetattr:device\n");
		res.status = NO_PSION;
		if (!query_devices()) {
			device *dp;

			for (dp = devices; dp; dp = dp->next) {
				if (debug)
					printf("\tcmp '%c', '%s'\n", dp->letter,
						inode->name);
				if (dp->letter == inode->name[0])
					break;
			}
			if (debug)
				printf("\tdevice: %s exists\n", (dp)?"":"not");
			if (dp) {
				res.status = NFS_OK;
				*fp = root_fattr;
				/* If it's writeable... */
				if (dp->attrib != 7)
					fp->mode |= 0200;
				fp->fileid = inode->inode;
				if (mp_dircount(inode, &dcount)) {
					res.status = psion_alive ? NFSERR_NOENT : NO_PSION;
					return &res;
				}
				if (fp->nlink != (dcount + 2))
					fp->mtime.seconds = time(0);
				fp->nlink = dcount + 2;
			}
		}
	} else {
		if (debug)
			printf("\tgetattr:fileordir\n");
		/* It's a normal file/dir */
		if (rfsv_getattr(inode->name, &pattr, &psize, &ptime)) {
			res.status = psion_alive ? NFSERR_NOENT : NO_PSION;
			return &res;
		}
		pattr2attr(pattr, psize, ptime, fp, (unsigned char *) fh->data);
		if (fp->type == NFDIR) {
			if (mp_dircount(inode, &dcount)) {
				res.status = psion_alive ? NFSERR_NOENT : NO_PSION;
				return &res;
			}
			if (fp->nlink != (dcount + 2))
				fp->mtime.seconds = time(0);
			fp->nlink = dcount + 2;
		}
	}
	add_cache(&attrcache, inode->inode, fp);
	return &res;
}

struct diropres *
nfsproc_lookup_2(diropargs *da)
{
	static struct diropres res;
	struct attrstat *gres;
	p_inode *inode = get_num(fh2inode(da->dir.data));
	char *fp = res.diropres_u.diropres.file.data;

	if (!inode) {
		if (debug)
			printf("lookup: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	if (debug)
		printf("\tlookup: in '%s'(%d) searching '%s'\n",
		       inode->name, inode->inode, da->name);

	if (inode->inode == root_fattr.fileid && !strcmp(da->name, "exit")) {
		exiting = 5;	/* Lets try it 5 times (10 sec) */
		res.status = NFSERR_EXIST;
		return &res;
	}
	if (inode->inode == root_fattr.fileid && !strcmp(da->name, "debug")) {
		debug = (debug + 1) & 3;	/* debug level of 0,1,2 & 3 */
		printf("Set debug level to %d\n", debug);
		res.status = NFSERR_EXIST;
		return &res;
	}
	if (!strcmp(da->name, "."))
		inode2fh(fh2inode(da->dir.data), fp);
	else if (!strcmp(da->name, ".."))
		inode2fh(getpinode(inode), fp);
	else
		inode2fh(get_nam(build_path(inode->name, da->name))->inode, fp);

	gres = nfsproc_getattr_2((struct nfs_fh *) fp);

	res.status = gres->status;
	res.diropres_u.diropres.attributes = gres->attrstat_u.attributes;
	return &res;
}

#define RA_MAXCOUNT ~0

static void
addentry(readdirargs *ra, entry ***where, int *searchi, int inode, char *name)
{
	int l, rndup;

	if (*searchi) {
		if (*searchi == inode)
			*searchi = 0;
		return;
	}
	if (ra->count == RA_MAXCOUNT)
		return;

	/*
	 * From ddan@au.stratus.com Wed Feb  8 04:14 MET 1995
	 * Modifed in pl7.a by Dan Danz to fix problem of missing files in readdir
	 *
	 * In a shotgun attempt at fixing this, I surmised that perhaps addentry
	 * was putting one too many entries in the result, so I increased
	 * the number of bytes for each entry by +8  ... my reasoning was that
	 * you need to account for the filename pointer and for the cookie int ...
	 * but in retrospect, I'm not sure about this reasoning.  HOWEVER, it appears
	 * that this fixes the problem.
	 * FIXED: See next comment (Rudi)
	 */

	/*
	 * Count the bytes needed for the xdr encoded data. Xdr is trickier than
	 * one (me :-) might think: Xdr converts a string into
	 * length (4 bytes) + data (rounded up to 4 bytes).
	 */
#define XDR_UNIT 4
	l = strlen(name);
	if ((rndup = l % XDR_UNIT) > 0)
		l += XDR_UNIT - rndup;
	l += XDR_UNIT;		/* Length of name */
	l += sizeof(entry);

	if (l > ra->count) {
		ra->count = RA_MAXCOUNT;
		return;
	}
	ra->count -= l;

	**where = (entry *) malloc(sizeof(entry));
	(**where)->fileid = inode;
	(**where)->name = (char *) strdup(name);
	*(int *) (**where)->cookie = inode;
	(**where)->nextentry = 0;
	*where = &(**where)->nextentry;
}

struct readdirres *
nfsproc_readdir_2(readdirargs *ra)
{
	static readdirres res;
	p_inode *inode = get_num(fh2inode(ra->dir.data));
	entry *centry, *fentry, **cp;
	char *bp;
	int searchinode;

	if (!inode) {
		if (debug)
			printf("readdir: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	cp = &res.readdirres_u.reply.entries;
	for (fentry = *cp; fentry; fentry = centry) {
		centry = fentry->nextentry;
		free(fentry->name);
		free(fentry);
	}
	*cp = 0;
	searchinode = *(int *) ra->cookie;

	if (debug)
		printf("\treaddir: %s, cookie:%x, count:%d\n",
		       inode->name, searchinode, ra->count);

/* . & .. */
	addentry(ra, &cp, &searchinode, inode->inode, ".");
	addentry(ra, &cp, &searchinode, getpinode(inode), "..");

	if (inode->inode == root_fattr.fileid) {	/* Root directory */
		device *dp;

		if (query_devices()) {
			res.status = NO_PSION;
			return &res;
		}
		for (dp = devices; dp; dp = dp->next) {
			char n[3];
			sprintf(n, "%c:", dp->letter);
			addentry(ra, &cp, &searchinode, get_nam(dp->name)->inode, n);
		}
	} else {
		dentry *e = NULL;
		if (debug)
			printf("\tnfsdir: dir\n");
		if (rfsv_dir(dirname(inode->name), &e)) {
			res.status = psion_alive ? NFSERR_NOENT : NO_PSION;
			return &res;
		}
		while (e) {
			fattr fp;
			dentry *o;
			int ni;

			bp = filname(e->name);
			ni = get_nam(build_path(inode->name, bp))->inode;
			addentry(ra, &cp, &searchinode, ni, (char *) bp);
			free(e->name);
			dpattr2attr(e->attr, e->size, e->time, &fp, ni);
			add_cache(&attrcache, ni, &fp);
			o = e;
			e = e->next;
			free(o);
		}
	}

	res.readdirres_u.reply.eof = ra->count == RA_MAXCOUNT ? 0 : 1;
	res.status = NFS_OK;
	return &res;
}

struct attrstat *
nfsproc_setattr_2(sattrargs *sa)
{
	static struct attrstat res;
	p_inode *inode = get_num(fh2inode(sa->file.data));
	fattr *fp;

	if (!inode) {
		if (debug)
			printf("setattr: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	if (debug)
		printf("\tsetattr %s called\n", inode->name);
	res = *nfsproc_getattr_2(&sa->file);
	if (res.status != NFS_OK)
		return &res;
	fp = &res.attrstat_u.attributes;

	if ((fp->type == NFREG) &&
	    (sa->attributes.size != -1) &&
	    (sa->attributes.size != fp->size)) {
		if (debug)
			printf("\t\tsetattr truncating to %d bytes\n", sa->attributes.size);
		if (rfsv_setsize(inode->name, sa->attributes.size) != 0) {
			res.status = psion_alive ? NFSERR_ROFS : NO_PSION;
			return &res;
		}
		fp->size = sa->attributes.size;
		rem_cache(&attrcache, inode->inode);
		add_cache(&attrcache, inode->inode, fp);
	}
	if ((sa->attributes.mtime.seconds != fp->mtime.seconds) &&
	    (sa->attributes.mtime.seconds != -1)) {
		if (rfsv_setmtime(inode->name, sa->attributes.mtime.seconds)) {
			res.status = (psion_alive) ? NFSERR_ACCES : NO_PSION;
			return &res;
		}
		fp->mtime.seconds = fp->atime.seconds = sa->attributes.mtime.seconds;
		rem_cache(&attrcache, inode->inode);
		add_cache(&attrcache, inode->inode, fp);
	}
	if ((sa->attributes.mode != fp->mode) &&
	    (sa->attributes.mode != -1)) {
		long psisattr, psidattr;
		attr2pattr(sa->attributes.mode, fp->mode, &psisattr, &psidattr);
		if (rfsv_setattr(inode->name, psisattr, psidattr)) {
			res.status = (psion_alive) ? NFSERR_ACCES : NO_PSION;
			return &res;
		}
		fp->mode = sa->attributes.mode;
		rem_cache(&attrcache, inode->inode);
		add_cache(&attrcache, inode->inode, fp);
	}
	res.status = NFS_OK;
	return &res;
}

static nfsstat *
remove_it(diropargs *da, int isdir)
{
	static nfsstat res;
	p_inode *inode = get_num(fh2inode(da->dir.data));
	long rfsv_res;

	if (!inode) {
		if (debug)
			printf("setattr: stale fh\n");
		res = NO_PSION;
		return &res;
	}
	if (debug)
		printf("\tremove_it: in %s: %s (%d)\n", inode->name, da->name, isdir);

	if (isdir)
		rfsv_res = rfsv_rmdir(build_path(inode->name, da->name));
	else
		rfsv_res = rfsv_remove(build_path(inode->name, da->name));
	if (rfsv_res != 0) {
		res = psion_alive ? NFSERR_ACCES : NO_PSION;
		return &res;
	}
	rem_cache(&attrcache, inode->inode);
	res = NFS_OK;
	return &res;
}

nfsstat *
nfsproc_remove_2(diropargs *da)
{
	return remove_it(da, 0);
}

nfsstat *
nfsproc_rmdir_2(diropargs *da)
{
	return remove_it(da, 1);
}

nfsstat *
nfsproc_rename_2(renameargs *ra)
{
	static nfsstat res;
	p_inode *from = get_num(fh2inode(ra->from.dir.data));
	p_inode *to = get_num(fh2inode(ra->to.dir.data));
	char ldata[300], *old, c;

	if (!from || !to) {
		if (debug)
			printf("rename: stale fh\n");
		res = NO_PSION;
		return &res;
	}
	strcpy(ldata + 1, build_path(to->name, ra->to.name));
	*ldata = strlen(ldata + 1);
	c = *ldata + 1;
	old = build_path(from->name, ra->from.name);

	if (debug)
		printf("\tRename: %s -> %s\n", old, ldata + 1);
	res = NFS_OK;
	if (rfsv_rename(old, ldata + 1)) {
		res = (psion_alive) ? NFSERR_ACCES : NO_PSION;
		return &res;
	}
	if (res == NFS_OK) {
		/* Preserve inode */
		strcpy((char *) ldata, build_path(to->name, ra->to.name));
		(void) re_nam(build_path(from->name, ra->from.name), ldata);
	}
	rem_cache(&attrcache, from->inode);
	rem_cache(&attrcache, to->inode);
	return &res;
}

/* ARGSUSED */
struct statfsres *
nfsproc_statfs_2(struct nfs_fh *fh)
{
	static statfsres res;
	statfsokres *rp;
	device *dp;

	if (debug)
		printf("\tstatfs..\n");

	rp = &res.statfsres_u.reply;
	rp->tsize = PBUFSIZE;
	rp->bsize = BLOCKSIZE;
	rp->blocks = rp->bfree = 0;
	res.status = NFS_OK;

	if (query_devices()) {
		/* Allow to mount it whithout the psion attached */
		if (psion_alive)
			return &res;	/* res.status = NO_PSION;  Hmm */
	}
	for (dp = devices; dp; dp = dp->next) {
		rp->blocks += (dp->total + BLOCKSIZE - 1) / BLOCKSIZE;
		rp->bfree += (dp->free + BLOCKSIZE - 1) / BLOCKSIZE;
	}
	rp->bavail = rp->bfree;

	return &res;
}

/*
 * Problem:
 * Since we are slow (Max 2Kbyte/s) and the biods are very impatient,
 * we receive each request (number of biods + 1 for the kernel itself)
 * times, this number can be lower for the last block. :-(
 * Solution: (not the best, probably)
 * Cache the read data. This cache will be invalidated if there are
 * no more requests in the queue for at least XXX seconds.
 * See also write :-(
 */
struct readres *
nfsproc_read_2(struct readargs *ra)
{
	static struct readres res;
	static unsigned char rop[NFS_MAXDATA];
	p_inode *inode = get_num(fh2inode(ra->file.data));
	fattr *fp;
	struct cache *cp;
	struct dcache *dcp;
	long phandle;
	long pattr;
	long psize;
	long ptime;
	int len;

	if (!inode) {
		if (debug)
			printf("read: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	if (debug)
		printf("\tread: %s off:%d count:%d\n", inode->name, ra->offset, ra->count);

	cp = search_cache(attrcache, inode->inode);
	if (cp && (dcp = search_dcache(cp, ra->offset, ra->count))) {
		if (debug)
			printf("\tread: dcache hit\n");
		res.readres_u.reply.attributes = cp->attr;
		bcopy(dcp->data, res.readres_u.reply.data.data_val, ra->count);
		res.readres_u.reply.data.data_len = ra->count;

		res.status = NFS_OK;
		return &res;
	}
	if (rfsv_fopen(1, inode->name, &phandle) != 0) {
		res.status = psion_alive ? NFSERR_NOENT : NO_PSION;
		return &res;
	}
	if (rfsv_read(rop, ra->offset,
		ra->count, phandle) < 0) {
		rfsv_fclose(phandle);
		res.status = NO_PSION;
		return &res;
	}
	rfsv_fclose(phandle);
	rfsv_getattr(inode->name, &pattr, &psize, &ptime);
	fp = &res.readres_u.reply.attributes;
	pattr2attr(pattr, psize, ptime, fp, (unsigned char *) ra->file.data);
	if (cp == 0)
		cp = add_cache(&attrcache, inode->inode, fp);

	len = fp->size - ra->offset;
	if (len > ra->count)
		len = ra->count;
	if (fp->size < ra->offset)
		len = 0;
	if (debug > 1)
		printf("Read: filesize %d read %d @ %d\n", fp->size, len, ra->offset);
	res.readres_u.reply.data.data_len = len;
	res.readres_u.reply.data.data_val = (char *) rop;

	if (len) {
		dcp = add_dcache(cp, ra->offset, ra->count, rop);
		dcp->towrite = 0;	/* don't write it back */
	}
	res.status = NFS_OK;
	return &res;
}


/*
 * Returns cachepointer on full hit, 0 on partial or no hit,
 * see below solaris comment
 */

static int
addwritecache(struct cache *cp, int doff, int dlen, unsigned char *dp)
{
	struct dcache *dcp;
	int len, changed, os, oe;
	unsigned char *pd, *pc;

	/*
	 * do the cachesearch: we are interested in partial hits, as we don't
	 * want to write anything twice (flash ram)
	 */
	for (dcp = cp->dcache; dcp; dcp = dcp->next)
		if (doff < dcp->offset + dcp->len && doff + dlen > dcp->offset)
			break;

	if (!dcp) {
		/* phew, nothing fancy to do */
		add_dcache(cp, doff, dlen, dp);
		return 0;
	}
	os = doff > dcp->offset ? doff : dcp->offset;
	oe = doff + dlen < dcp->offset + dcp->len ? doff + dlen : dcp->offset + dcp->len;
	pd = dp + os - doff;
	pc = dcp->data + os - dcp->offset;
	len = oe - os;

	changed = 0;
	if (bcmp(pd, pc, len)) {
		bcopy(pd, pc, len);
		dcp->towrite = 1;
		changed = 1;
	}
	if (doff >= dcp->offset && doff + dlen <= dcp->offset + dcp->len) {
		if (debug)
			printf("\twrite: full cache hit\n");
		return !changed;
	}
	if (debug)
		printf("\twrite: partial cache hit (off %d len %d)\n", dcp->offset, dcp->len);

	/* Do we have some data below the cached area... */
	if (doff < dcp->offset)
		(void) addwritecache(cp, doff, dcp->offset - doff, dp);

	/* ...or some above? */
	len = (doff + dlen) - (dcp->offset + dcp->len);
	if (len > 0)
		(void) addwritecache(cp, dcp->offset + dcp->len, len, dp + dlen - len);

	return 0;
}



/*
 * The same problem here as above: we receive numerous requests,
 * not even in a specified order. The only good thing is that each request
 * up to the last is exactly the same size.
 * A new problem:
 * Since I dunno how to seek to a point beyond the end of the file (psion),
 * I can't sopport the exact semantics of write. Such files will never be
 * written completely. :-(
 *
 * Another dumb solaris (sysv?) problem: if the client is writing 512 byte
 * blocks we receive following write requests:
 * off 0 len 512, off 0 len 1024, off 0 len 1536, ... so on till len 4096,
 * that means 4 times as much data as actually needed.
 * We should check if the block was partially written, and write only the
 * difference
 */
struct attrstat *
nfsproc_write_2(writeargs *wa)
{
	static struct attrstat res;
	p_inode *inode = get_num(fh2inode(wa->file.data));
	struct cache *cp;
	struct dcache *dcp;
	fattr *fp;
	struct attrstat *gres;
	long phandle;
	int len, dlen, doff;

	if (!inode) {
		if (debug)
			printf("write: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	if (debug)
		printf("\twrite:%s off:%d l:%d\n", inode->name, wa->offset, wa->data.data_len);

	dlen = wa->data.data_len;
	doff = wa->offset;

	/* fetch attributes */
	if ((cp = search_cache(attrcache, inode->inode)) == 0) {
		gres = nfsproc_getattr_2((struct nfs_fh *) wa->file.data);
		if (gres->status != NFS_OK) {
			res.status = gres->status;
			return &res;
		}
		cp = search_cache(attrcache, inode->inode);
		if (!cp)
			abort();
	}
	fp = &cp->attr;
	if (fp->size < doff + dlen)
		fp->size = doff + dlen;
	fp->blocks = (fp->size + (BLOCKSIZE - 1)) / BLOCKSIZE;
	fp->atime.seconds = fp->mtime.seconds = fp->ctime.seconds = time(0);

	res.attrstat_u.attributes = *fp;

	if (addwritecache(cp, doff, dlen, (unsigned char *) wa->data.data_val)) {
		res.status = NFS_OK;
		return &res;
	}
/* Write out as many blocks from the cache as we can */
	for (;;) {
		if (debug > 2)
			for (dcp = cp->dcache; dcp; dcp = dcp->next)
				printf("\t\tCheck: %d=%d,%d,%d>=%d\n",
				   inode->inode, cp->inode, dcp->towrite,
				       cp->actual_size, dcp->offset);
		for (dcp = cp->dcache; dcp; dcp = dcp->next)
			if (dcp->towrite && cp->actual_size >= dcp->offset)
				break;
		if (!dcp)	/* Can't write any blocks */
			break;

		if (debug)
			printf("\twriting off: %d, len: %d, act: %d\n",
			       dcp->offset, dcp->len, cp->actual_size);

		if (rfsv_fopen(0x200, inode->name, &phandle) != 0) {
			if (debug)
				printf("write: open failed\n");
			res.status = psion_alive ? NFSERR_NOSPC : NO_PSION;
			return &res;
		}
		if (rfsv_write(dcp->data, dcp->offset, dcp->len, phandle) != dcp->len) {
			rfsv_fclose(phandle);
			if (debug)
				printf("write: dump failed\n");
			res.status = psion_alive ? NFSERR_NOSPC : NO_PSION;
			return &res;
		}
		rfsv_fclose(phandle);
		dcp->towrite = 0;
		len = dcp->offset + dcp->len;
		if (len > cp->actual_size)
			cp->actual_size = len;
		if (debug)
			printf("\twritten: new length: %d\n", cp->actual_size);
	}

	if (debug)
		printf("\twrite -> ISOK (%d, %d %d)\n",
		       res.attrstat_u.attributes.size,
		       res.attrstat_u.attributes.fileid,
		       res.attrstat_u.attributes.fsid);
	res.status = NFS_OK;
	return &res;
}

/* Dummy routines */
void *
nfsproc_writecache_2()
{
	static char res;
	if (debug)
		printf("writecache???\n");
	res = (char) NFSERR_FBIG;
	return (void *) &res;
}

void *
nfsproc_null_2()
{
	static char res;
	if (debug)
		printf("null.\n");
	res = (char) NFSERR_FBIG;
	return (void *) &res;
}

void *
nfsproc_root_2()
{
	static char res;
	if (debug)
		printf("root????\n");
	res = (char) NFSERR_FBIG;
	return (void *) &res;
}

/*
 * Links and symlinks are not supported on Psion
 */

/* ARGSUSED */
nfsstat *
nfsproc_link_2(linkargs *la)
{
	static nfsstat res;

	if (debug)
		printf("link..\n");
	res = NFSERR_ACCES;
	return &res;
}

/* ARGSUSED */
struct readlinkres *
nfsproc_readlink_2(struct nfs_fh *fh)
{
	static readlinkres res;

	if (debug)
		printf("readlink...\n");
	res.status = NFSERR_ACCES;
	return &res;
}

/* ARGSUSED */
nfsstat *
nfsproc_symlink_2(symlinkargs *sa)
{
	static nfsstat res;

	if (debug)
		printf("symlink..\n");
	res = NFSERR_ACCES;
	return &res;
}
