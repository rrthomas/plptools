/* $Id$
 *
 * Original version of this file from p3nfsd-5.4 by
 * Rudolf Koenig (rfkoenig@immd4.informatik.uni-erlangen.de)
 *
 * Modifications for plputils by Fritz Elfert <felfert@to.com>
 *
 */
#include "OSdefs.h"
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#if defined(__SVR4) || defined(__GLIBC__) || defined(__NetBSD__)
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif
#include "nfs_prot.h"
#include "mp.h"
#include "rfsv_api.h"
#include "builtins.h"

static device *devices;
struct cache *attrcache = NULL;

long param_read(builtin_node *node, char *buf, unsigned long offset, long len) {
	char tmp[10];
	unsigned long val = 0;

	if (!strcmp(node->name, "acache"))
		val = cache_keep;
	if (!strcmp(node->name, "dcache"))
		val = devcache_keep;
	if (!strcmp(node->name, "debuglevel"))
		val = debug;
	sprintf(tmp, "%ld\n", val);
	if (offset >= strlen(tmp))
		return 0;
	strncpy(buf, &tmp[offset], len);
	return strlen(buf);
}

long param_write(builtin_node *node, char *buf, unsigned long offset, long len) {
	unsigned long val;

	if (len > 1 && offset == 0) {
		int res = sscanf(buf, "%ld", &val);
		if (res == 1) {
			if (!strcmp(node->name, "acache"))
				cache_keep = val;
			if (!strcmp(node->name, "dcache"))
				devcache_keep = val;
			if (!strcmp(node->name, "debuglevel")) {
				if (val != debug) {
					if (val > debug)
						debug = val;
					debuglog("Set debug level to %d\n", val);
					debug = val;
				}
			}
		}
	}
	return len;
}

long param_getsize(builtin_node *node) {
	char tmp[10];
	return param_read(node, tmp, 0, sizeof(tmp));
}

long exit_write(builtin_node *node, char *buf, unsigned long offset, long len) {
	if (len >= 4 && offset == 0) {
		if (!strncmp(buf, "stop", 4))
			exiting = 5; /* Lets try it 5 times (10 sec) */
	}
	return len;
}

long exit_read(builtin_node *node, char *buf, unsigned long offset, long len) {
	if (offset > 4)
		return 0;
	if ((len + offset) > 5)
		len = 5 - offset;
	strncpy(buf, "----\n", len);
	return strlen(buf);
}

long user_write(builtin_node *node, char *buf, unsigned long offset, long len) {
	char tmp[256];
	if (len > 1 && offset == 0) {
		char *p;

		if (len > (sizeof(tmp) - 1))
			len = sizeof(tmp) - 1;
		strncpy(tmp, buf, len);
		debuglog("pwrite: %d\n", len);
		tmp[len] = '\0';
		if ((p = strchr(tmp, '\n')))
			*p = '\0';
		set_owner(tmp, 0);
	}
	return len;
}

long user_read(builtin_node *node, char *buf, unsigned long offset, long len) {
	struct passwd *pw = getpwuid(root_fattr.uid);
	char tmp[255];

	if (pw)
		sprintf(tmp, "%s\n", pw->pw_name);
	else
		sprintf(tmp, "???\n");
	endpwent();
	if (offset >= strlen(tmp))
		return 0;
	strncpy(buf, &tmp[offset], len);
	return strlen(buf);
}

long user_getsize(builtin_node *node) {
	char tmp[255];
	return user_read(node, tmp, 0, sizeof(tmp));
}

static long generic_sattr(builtin_node *p, unsigned long sa, unsigned long da) {
	p->attr |= sa;
	p->attr &= ~da;
	return 0;
}

long generic_getlinks(builtin_node *node) {
	builtin_child *cp = node->childs;
	long ncount = 0;

	while (cp) {
		if ((cp->node->flags & BF_EXISTS_ALWAYS) || rfsv_isalive())
			ncount++;
		cp = cp->next;
	}
	return ncount;
}

long generic_getdents(builtin_node *node, dentry **e) {
	builtin_child *cp = node->childs;

	while (cp) {
		if ((cp->node->flags & BF_EXISTS_ALWAYS) || rfsv_isalive()) {
			dentry *tmp = (dentry *)malloc(sizeof(dentry));
			if (!tmp)
				return -1;
			tmp->time = time(0);
			tmp->size = cp->node->getsize ? cp->node->getsize(cp->node) : cp->node->size;
			tmp->attr = cp->node->attr;
			tmp->name = strdup(cp->node->name);
			tmp->next = *e;
			*e = tmp;
		}
		cp = cp->next;
	}
	return 0;
}

static builtin_node *builtins = NULL;
static void dump_proctree(builtin_node *n);

static void clear_procs(builtin_child **childs) {
	builtin_child **cp = childs;

	debuglog("Before clear_procs\n");
	dump_proctree(builtins);
	while (*cp) {
		if ((*cp)->node->flags & BF_ISPROCESS) {
			builtin_child *tmp = *cp;
			*cp = (*cp)->next;
			unregister_builtin(tmp->node);
			free(tmp);
		} else
			cp = &((*cp)->next);
	}
	debuglog("After clear_procs\n");
	dump_proctree(builtins);
}

static time_t procs_stamp = 0;
static long procs_keep = 10;

static long proc_getlinks(builtin_node *node) {
	builtin_child *cp;
	long ncount = 0;

	if ((time(0) - procs_stamp) > procs_keep) {
		debuglog("PROCESSLIST\n");
		clear_procs(&node->childs);
		rpcs_ps();
		procs_stamp = time(0);
		debuglog("After rpcs_ps\n");
		dump_proctree(builtins);
	}
	cp = node->childs;
	while (cp) {
		if ((cp->node->flags & BF_EXISTS_ALWAYS) || rfsv_isalive())
			ncount++;
		cp = cp->next;
	}
	return ncount;
}

static long proc_getdents(builtin_node *node, dentry **e) {
	builtin_child *cp = node->childs;

	while (cp) {
		if ((cp->node->flags & BF_EXISTS_ALWAYS) || rfsv_isalive()) {
			dentry *tmp = (dentry *)malloc(sizeof(dentry));
			if (!tmp)
				return -1;
			tmp->time = time(0);
			tmp->size = cp->node->getsize ? cp->node->getsize(cp->node) : cp->node->size;
			tmp->attr = cp->node->attr;
			tmp->name = strdup(cp->node->name);
			tmp->next = *e;
			*e = tmp;
		}
		cp = cp->next;
	}
	return 0;
}


static builtin_node proc_node =	{
	NULL, NULL, NULL, NULL, "proc",  BF_EXISTS_ALWAYS|BF_NOCACHE, PSI_A_DIR, 0, NULL, NULL, NULL, NULL, proc_getlinks, proc_getdents
};

static builtin_node fixed_builtins[] = {
	{ NULL, NULL, NULL, NULL, "owner", 0, PSI_A_READ | PSI_A_RDONLY, 0, rpcs_ownerSize, rpcs_ownerRead, NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, "debuglevel", BF_EXISTS_ALWAYS|BF_NOCACHE, PSI_A_READ, 0, param_getsize, param_read, param_write, generic_sattr, NULL, NULL },
	{ NULL, NULL, NULL, NULL, "acache",     BF_EXISTS_ALWAYS|BF_NOCACHE, PSI_A_READ, 0, param_getsize, param_read, param_write, generic_sattr, NULL, NULL },
	{ NULL, NULL, NULL, NULL, "dcache",     BF_EXISTS_ALWAYS|BF_NOCACHE, PSI_A_READ, 0, param_getsize, param_read, param_write, generic_sattr, NULL, NULL },
	{ NULL, NULL, NULL, NULL, "unixowner",  BF_EXISTS_ALWAYS|BF_NOCACHE, PSI_A_READ, 0, user_getsize, user_read, user_write, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, "exit",       BF_EXISTS_ALWAYS|BF_NOCACHE, PSI_A_READ, 5, NULL, exit_read, exit_write, NULL, NULL, NULL },
};
static int num_fixed_builtins = sizeof(fixed_builtins) / sizeof(builtin_node);

static void dump_proctree(builtin_node *n) {
	while (n) {
		if (n->name)
			debuglog("node@%p, \"%s\"\n", n, n->name);
		else
			debuglog("node@%p, \"(null)\"\n", n);
		if (n->childs) {
			builtin_child *c = n->childs;
			debuglog("Childs:\n");
			while (c) {
				if (c->node)
					debuglog("  %s\n", c->node->name ? c->node->name : "(null)");
				c = c->next;
			}
		}
		if (n->parent)
			debuglog("Parent: %s\n", n->parent->name);
		n = n->next;
	}
}

static int add_child(builtin_child **childs, builtin_node *node) {
	builtin_child *newc = malloc(sizeof(builtin_child));
	if (!newc) {
		errorlog("Out of memory in add_child %s\n", node->name);
		return -1;
	}
	newc->next = *childs;
	newc->node = node;
	*childs = newc;
	return 0;
}

static int remove_child(builtin_child **childs, builtin_node *node) {
	builtin_child **cp = childs;

	if (debug)
		debuglog("remove_child %s\n", node->name);
	while (*cp) {
		if ((*cp)->node == node) {
			builtin_child *tmp = *cp;
			*cp = (*cp)->next;
			free(tmp);
			return 0;
		}
		cp = &((*cp)->next);
	}
	return -1;
}

char *builtin_path(builtin_node *node) {
	static char tmp[1024];
	char tmp2[1024];

	strcpy(tmp, node->name);
	node = node->parent;
	while (node) {
		sprintf(tmp2, "%s\\%s", node->name, tmp);
		strcpy(tmp, tmp2);
		node = node->parent;
	}
	return tmp;
}

int unregister_builtin(builtin_node *node) {
	builtin_child *cp = node->childs;
	builtin_node **n;

	/**
	 * Unlink and free childs.
	 */
	if (debug)
		debuglog("unregister_builtin %s\n", node->name);

	while (cp) {
		builtin_child *old = cp;
		cp->node->parent = 0L;
		unregister_builtin(cp->node);
		cp = cp->next;
		free(old);
	}
	/**
	 * Unlink ourselves from parent child list.
	 */
	if (node->parent)
		remove_child(&node->parent->childs, node);
	n = &builtins;
	while (*n) {
		if (*n == node) {
			*n = node->next;
			break;
		} else
			n = &(*n)->next;
	}
	if (node->name)
		free(node->name);
	if (node->private_data)
		free(node->private_data);
	free(node);
	return 0;
}

builtin_node *register_builtin(char *parent, builtin_node *node) {
	builtin_node *bn;
	builtin_node *parent_node = 0L;

	debuglog("register_builtin node=%p\n", node);
	if (!node) {
		errorlog("register_builtin called with NULL node\n");
		return NULL;
	}
	if (!node->name) {
		errorlog("register_builtin called without name\n");
		return NULL;
	}
	if (parent) {
		debuglog("register_builtin parent: %s\n", parent);
		for (bn = builtins; bn; bn = bn->next) {
			debuglog("cmp parent: %s %s\n", builtin_path(bn), parent);
			if (!strcmp(builtin_path(bn), parent)) {
				debuglog("cmp parent found bn=%s\n", bn->name);
				break;
			}
		}
		if (!bn) {
			errorlog("register_builtin for %s called with nonexistent parent %s\n", node->name, parent);
			return NULL;
		}
		parent_node = bn;
	}
	bn = malloc(sizeof(builtin_node));
	if (!bn) {
		errorlog("out of memory while registering builtin %s\n", node->name);
		return NULL;
	}
	memset(bn, 0, sizeof(builtin_node));
	bn->name = strdup(node->name);
	if (!bn->name) {
		errorlog("out of memory while registering builtin %s\n", node->name);
		free(bn);
		return NULL;
	}
	if (parent_node)
		debuglog("register_builtin %s in %s\n", node->name, builtin_path(parent_node));
	else
		debuglog("register_builtin %s\n", node->name);
	bn->parent = parent_node;
	bn->flags = node->flags;
	bn->attr = node->attr;
	bn->size = node->size;
	bn->getsize = node->getsize;
	bn->read = node->read;
	bn->write = node->write;
	bn->sattr = node->sattr;
	bn->getlinks = node->getlinks;
	bn->getdents = node->getdents;
	bn->private_data = node->private_data;
	if (parent_node) {
		debuglog("Add child %s in %s\n", bn->name, bn->parent->name);
		if (add_child(&(parent_node->childs), bn)) {
			errorlog("Couldn't add child %s\n", bn->name);
			free(bn->name);
			free(bn);
			return NULL;
		}
	}
	bn->next = builtins;
	builtins = bn;
	debuglog("After register_builtin\n");
	dump_proctree(builtins);
	debuglog("register_builtin new node=%p\n", bn);
	return bn;
}

/*
 * nfsd returned NFSERR_STALE if the Psion wasn't present, but I didn't like
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
	u_int32_t phandle;
	int rfsv_ret;

	debuglog("create: in %s %s (%#o, %d)\n",
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
		res.status = rfsv_isalive() ? NFSERR_NAMETOOLONG : NO_PSION;
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
	if (rfsv_isalive())
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
	if ((oattr & 0400) != (nattr & 0400)) {
		if (nattr & 0400)		/* readable */
			*psidattr |= PSI_A_READ;
		else
			*psisattr |= PSI_A_READ;
	}
	if ((oattr & 0200) != (nattr & 0200)) {
		if (nattr & 0200)		/* readonly */
			*psidattr |= PSI_A_RDONLY;
		else
			*psisattr |= PSI_A_RDONLY;
	}
	if ((oattr & 0020) != (nattr & 0020)) {
		if (nattr & 0020)	/* group-write    -> archive */
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
pattr2attr(long psiattr, long size, long ftime, fattr *fp, int inode)
{
	bzero((char *) fp, sizeof(*fp));

	if (psiattr & PSI_A_DIR) {
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
		if (psiattr & PSI_A_READ)
			fp->mode |= 0400;	/* File readable (?) */
		if (!(psiattr & PSI_A_RDONLY))
			fp->mode |= 0200;	/* File writeable  */
		/* fp->mode |= 0100;		   File executable */
		if (!(psiattr & PSI_A_HIDDEN))
			fp->mode |= 0004;	/* Not Hidden  <-> world read */
		if (psiattr & PSI_A_SYSTEM)
			fp->mode |= 0002;	/* System      <-> world write */
		if (psiattr & PSI_A_VOLUME)
			fp->mode |= 0001;	/* Volume      <-> world exec */
		if (psiattr & PSI_A_ARCHIVE)
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

static int proc_top = 0;

static int
query_devices()
{
	device *dp, *np;
	int link_count = 2;	/* set the root link count */

	if (!proc_top) {
		int i;
		if (register_builtin(NULL, &proc_node)) {
			for (i = 0; i < num_fixed_builtins; i++)
				if (register_builtin(proc_node.name, &fixed_builtins[i]))
					proc_top++;
		} else
			errorlog("Couldn't register /proc\n");
		if (proc_top != num_fixed_builtins)
			proc_top = 0;
	}
	if (query_cache)
		return 0;
	for (dp = devices; dp; dp = np) {
		np = dp->next;
		free(dp->name);
		free(dp);
	}
	devices = 0;
	debuglog("RFSV drivelist\n");
	if (rfsv_drivelist(&link_count, &devices))
		return 1;
	query_cache = 1;
	devcache_stamp = time(0);
	root_fattr.nlink = link_count + 1;
	return 0;
}

static int
mp_dircount(p_inode *inode, long *count)
{
	dentry *e = NULL;
	long ret;
	builtin_node *bn;

	*count = 0;
	debuglog("dircount: %s\n", inode->name);
	for (bn = builtins; bn; bn = bn->next) {
		if ((!strcmp(builtin_path(bn), inode->name) && (bn->attr & PSI_A_DIR))) {
			if (bn->getlinks)
				*count = bn->getlinks(bn);
			return 0;
		}
	}
	debuglog("RFSV dir %s\n", inode->name);
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
			pattr2attr(e->attr, e->size, e->time, &fp, ni);
			if (rfsv_isalive())
				add_cache(&attrcache, ni, &fp);
		}
		o = e;
		e = e->next;
		free(o);
		if (fp.type == NFDIR)
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
	int builtin = 0;
	int l;

	debuglog("getattr: '%s', %d\n", inode->name, inode->inode);
	res.status = NFS_OK;

	if ((cp = search_cache(attrcache, inode->inode))) {
		debuglog("getattr: cache hit\n");
		*fp = cp->attr;	/* gotcha */
		if (fp->nlink > 0)
			return &res;
	}
	l = strlen(inode->name);

	if (inode->inode == root_fattr.fileid) {
		/* It's the root inode */
		debuglog("getattr: root inode (%#o)\n", root_fattr.mode);

		if (query_devices())	/* root inode and proc is always there */
			root_fattr.nlink = 3;
		*fp = root_fattr;
	} else if (l == 2 && inode->name[1] == ':') {
		debuglog("getattr: device\n");
		res.status = NO_PSION;
		if (!query_devices()) {
			device *dp;

			for (dp = devices; dp; dp = dp->next) {
				debuglog("cmp '%c', '%s'\n", dp->letter,
						inode->name);
				if (dp->letter == inode->name[0])
					break;
			}
			debuglog("device: %s\n", dp ? "exists" : "does not exist");
			if (dp) {
				res.status = NFS_OK;
				*fp = root_fattr;
				/* If it's writeable... */
				if (dp->attrib != 7)
					fp->mode |= 0200;
				fp->fileid = inode->inode;
				if (mp_dircount(inode, &dcount)) {
					res.status = rfsv_isalive() ? NFSERR_NOENT : NO_PSION;
					return &res;
				}
				if (fp->nlink != dcount + 2)
					fp->mtime.seconds = time(0);
				fp->nlink = dcount + 2;
			}
		}
	} else {
		builtin_node *bn;

		for (bn = builtins; bn; bn = bn->next) {
			if (!strcmp(inode->name, builtin_path(bn))) {
				pattr = bn->attr;
				if (pattr & PSI_A_DIR)
					psize = 0;
				else
					psize = (bn->getsize) ? bn->getsize(bn) : bn->size;
				ptime = time(0);
				res.status = NFS_OK;
				builtin = 1;
				if (bn->flags && BF_NOCACHE)
					builtin++;
				break;
			}
		}

		if (!builtin) {
			debuglog("getattr: fileordir\n");
			/* It's a normal file/dir */
			debuglog("RFSV getattr %s\n", inode->name);
			if (rfsv_getattr(inode->name, &pattr, &psize, &ptime)) {
				res.status = rfsv_isalive() ? NFSERR_NOENT : NO_PSION;
				return &res;
			}
		}
		pattr2attr(pattr, psize, ptime, fp, fh2inode((char *) fh->data));
		if (fp->type == NFDIR) {
			if (mp_dircount(inode, &dcount)) {
				res.status = rfsv_isalive() ? NFSERR_NOENT : NO_PSION;
				return &res;
			}
			if (fp->nlink != dcount + 2)
				fp->mtime.seconds = time(0);
			fp->nlink = dcount + 2;
		}
	}
	if (rfsv_isalive() && builtin < 2)
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
		debuglog("lookup: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	debuglog("lookup: in '%s' (%d) finding '%s'\n",
		       inode->name, inode->inode, da->name);
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
		debuglog("readdir: stale fh\n");
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

	debuglog("readdir: %s, cookie:%x, count:%d\n",
		       inode->name, searchinode, ra->count);

	/* . & .. */
	addentry(ra, &cp, &searchinode, inode->inode, ".");
	addentry(ra, &cp, &searchinode, getpinode(inode), "..");

	if (inode->inode == root_fattr.fileid) {	/* Root directory */
		device *dp;

		addentry(ra, &cp, &searchinode, get_nam("/proc")->inode, "proc");
		if (query_devices()) {
			res.readdirres_u.reply.eof = ra->count == RA_MAXCOUNT ? 0 : 1;
			res.status = NFS_OK;
			debuglog("readdir: eof=%d\n", res.readdirres_u.reply.eof);
			return &res;
		}
		for (dp = devices; dp; dp = dp->next) {
			char n[3];
			sprintf(n, "%c:", dp->letter);
			addentry(ra, &cp, &searchinode, get_nam(dp->name)->inode, n);
		}
	} else {
		builtin_node *bn;
		int builtin = 0;
		dentry *e = NULL;
		debuglog("nfsdir: dir\n");

		for (bn = builtins; bn ; bn = bn->next)
			if (!strcmp(inode->name, builtin_path(bn))) {
				if (bn->attr & PSI_A_DIR) {
					if (bn->getdents)
						bn->getdents(bn, &e);
					builtin = 1;
				} else {
					res.status = NFSERR_NOTDIR;
					return &res;
				}
			}
		if (!builtin) {
			debuglog("RFSV dir2 %s\n", inode->name);
			if (rfsv_dir(dirname(inode->name), &e)) {
				res.status = rfsv_isalive() ? NFSERR_NOENT : NO_PSION;
				return &res;
			}
		}
		while (e) {
			fattr fp;
			dentry *o;
			int ni;
	
			bp = filname(e->name);
			ni = get_nam(build_path(inode->name, bp))->inode;
			addentry(ra, &cp, &searchinode, ni, (char *) bp);
			free(e->name);
			pattr2attr(e->attr, e->size, e->time, &fp, ni);
			if (rfsv_isalive())
				add_cache(&attrcache, ni, &fp);
			o = e;
			e = e->next;
			free(o);
		}
	}

	res.readdirres_u.reply.eof = ra->count == RA_MAXCOUNT ? 0 : 1;
	res.status = NFS_OK;
	debuglog("readdir: eof=%d\n", res.readdirres_u.reply.eof);
	return &res;
}

struct attrstat *
nfsproc_setattr_2(sattrargs *sa)
{
	static struct attrstat res;
	p_inode *inode = get_num(fh2inode(sa->file.data));
	fattr *fp;
	builtin_node *bn;
	int builtin = 0;

	if (!inode) {
		debuglog("setattr: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	debuglog("setattr '%s'\n", inode->name);
	res = *nfsproc_getattr_2(&sa->file);
	if (res.status != NFS_OK)
		return &res;
	fp = &res.attrstat_u.attributes;

	for (bn = builtins; bn; bn = bn->next)
		if (!strcmp(inode->name, builtin_path(bn))) {
			builtin = 1;
			break;
		}

	if ((fp->type == NFREG) &&
	    (sa->attributes.size != -1) &&
	    (sa->attributes.size != fp->size)) {
		debuglog("RFSV setsize %s %d\n", inode->name, sa->attributes.size);
		if (builtin) {
			if (bn->attr & PSI_A_RDONLY) {
				res.status = NFSERR_ACCES;
				return &res;
			}
		} else {
			if (rfsv_setsize(inode->name, sa->attributes.size) != 0) {
				// FIXME: more different error codes
				res.status = rfsv_isalive() ? NFSERR_ROFS : NO_PSION;
				return &res;
			}
		}
		fp->size = sa->attributes.size;
		rem_cache(&attrcache, inode->inode);
		if (rfsv_isalive())
			add_cache(&attrcache, inode->inode, fp);
	}
	if ((sa->attributes.mtime.seconds != fp->mtime.seconds) &&
	    (sa->attributes.mtime.seconds != -1) && !builtin) {
		debuglog("RFSV setmtime %s %d\n", inode->name, sa->attributes.mtime.seconds);
		if (rfsv_setmtime(inode->name, sa->attributes.mtime.seconds)) {
			res.status = (rfsv_isalive()) ? NFSERR_ACCES : NO_PSION;
			return &res;
		}
		fp->mtime.seconds = fp->atime.seconds = sa->attributes.mtime.seconds;
		rem_cache(&attrcache, inode->inode);
		if (rfsv_isalive())
			add_cache(&attrcache, inode->inode, fp);
	}
	if ((sa->attributes.mode != (fp->mode & ~NFSMODE_MASK)) &&
	    (sa->attributes.mode != -1)) {
		long psisattr, psidattr;
		attr2pattr(sa->attributes.mode, fp->mode, &psisattr, &psidattr);
		debuglog("RFSV setattr '%s' %d %d\n", inode->name, psisattr, psidattr);
		if (builtin) {
			if ((bn->sattr == NULL) || bn->sattr(bn, psisattr, psidattr)) {
				res.status = NFSERR_ACCES;
				return &res;
			}
		} else {
			if (rfsv_setattr(inode->name, psisattr, psidattr)) {
				res.status = (rfsv_isalive()) ? NFSERR_ACCES : NO_PSION;
				return &res;
			}
		}
		debuglog("changing mode from %o to %o\n", fp->mode, (fp->mode & NFSMODE_MASK) | sa->attributes.mode);
		fp->mode = (fp->mode & NFSMODE_MASK) | sa->attributes.mode;
		rem_cache(&attrcache, inode->inode);
		if (rfsv_isalive() && !builtin)
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
		debuglog("remove_it: stale fh\n");
		res = NO_PSION;
		return &res;
	}
	debuglog("remove_it: in %s: %s (%d)\n", inode->name, da->name, isdir);

	if (isdir) {
		debuglog("RFSV rmdir %s\n", build_path(inode->name, da->name));
		rfsv_res = rfsv_rmdir(build_path(inode->name, da->name));
	} else {
		debuglog("RFSV remove %s\n", build_path(inode->name, da->name));
		rfsv_res = rfsv_remove(build_path(inode->name, da->name));
	}
	if (rfsv_res != 0) {
		res = rfsv_isalive() ? NFSERR_ACCES : NO_PSION;
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
	/* FIXME: Buffer overflow */
	char ldata[300], *old, c;

	if (!from || !to) {
		debuglog("rename: stale fh\n");
		res = NO_PSION;
		return &res;
	}
	strcpy(ldata + 1, build_path(to->name, ra->to.name));
	*ldata = strlen(ldata + 1);
	c = *ldata + 1;
	old = build_path(from->name, ra->from.name);

	res = NFS_OK;
	debuglog("RFSV rename %s -> %s\n", old, ldata + 1);
	if (rfsv_rename(old, ldata + 1)) {
		res = (rfsv_isalive()) ? NFSERR_ACCES : NO_PSION;
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

	debuglog("statfs..\n");

	rp = &res.statfsres_u.reply;
	rp->tsize = PBUFSIZE;
	rp->bsize = BLOCKSIZE;
	rp->blocks = rp->bfree = 0;
	res.status = NFS_OK;

	if (query_devices()) {
		/* Allow to mount it without the psion attached */
		if (rfsv_isalive())
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
	long pattr;
	long psize;
	long ptime;
	int len = 0;
	builtin_node *bn;
	int builtin = 0;

	if (!inode) {
		debuglog("read: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	debuglog("read: %s off:%d count:%d\n", inode->name, ra->offset, ra->count);

	cp = search_cache(attrcache, inode->inode);
	if (cp && (dcp = search_dcache(cp, ra->offset, ra->count))) {
		debuglog("read: dcache hit\n");
		res.readres_u.reply.attributes = cp->attr;
		bcopy(dcp->data, res.readres_u.reply.data.data_val, ra->count);
		res.readres_u.reply.data.data_len = ra->count;

		res.status = NFS_OK;
		return &res;
	}

	debuglog("RFSV read %s\n", inode->name);

	for (bn = builtins; bn; bn = bn->next) {
		if (!strcmp(inode->name, builtin_path(bn))) {
			if (bn->attr & PSI_A_DIR) {
				res.status = NFSERR_ISDIR;
				return &res;
			}
			if (bn->read) {
				len = bn->read(bn, rop, ra->offset, ra->count);
				builtin = 1;
				break;
			} else {
				res.status = NFSERR_IO;
				return &res;
			}
		}
	}

	if (!builtin) {
		if (rfsv_read(rop, ra->offset,
			ra->count, inode->name) < 0) {
			res.status = rfsv_isalive() ? NFSERR_NOENT : NO_PSION;
			return &res;
		}
	}
	fp = &res.readres_u.reply.attributes;
	if (!cp) {
		// Problem: if an EPOC process is enlarging the file, we won't recognize it
		debuglog("RFSV getattr '%s'\n", inode->name);
		if (builtin) {
			pattr = bn->attr;
			psize = (bn->getsize) ? bn->getsize(bn) : bn->size;
			ptime = time(0);
		} else
			rfsv_getattr(inode->name, &pattr, &psize, &ptime);
		pattr2attr(pattr, psize, ptime, fp, fh2inode((char *)ra->file.data));
		cp = add_cache(&attrcache, inode->inode, fp);
	} else {
		*fp = cp->attr;
	}
	
	len = cp->actual_size - ra->offset;
	if (len > ra->count)
		len = ra->count;
	if (cp->actual_size < ra->offset)
		len = 0;
	if (debug > 1)
		debuglog("Read: filesize %d read %d @ %d\n", cp->actual_size, len, ra->offset);
	if (len) {
		dcp = add_dcache(cp, ra->offset, ra->count, rop);
		dcp->towrite = 0;	/* don't write it back */
	}
	if (builtin)
		rem_cache(&attrcache, inode->inode);
	res.readres_u.reply.data.data_len = len;
	res.readres_u.reply.data.data_val = (char *) rop;

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
		debuglog("write: full cache hit\n");
		return !changed;
	}
	debuglog("write: partial cache hit (off %d len %d)\n", dcp->offset, dcp->len);

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
 * I can't support the exact semantics of write. Such files will never be
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
	int len = 0;
	int dlen, doff;
	builtin_node *bn;

	if (!inode) {
		debuglog("write: stale fh\n");
		res.status = NO_PSION;
		return &res;
	}
	debuglog("write:%s off:%d l:%d\n", inode->name, wa->offset, wa->data.data_len);

	dlen = wa->data.data_len;
	doff = wa->offset;

	for (bn = builtins; bn ; bn = bn->next) {
		if (!strcmp(inode->name, builtin_path(bn))) {
			if (bn->attr & PSI_A_DIR)
				res.status = NFSERR_ISDIR;
			else {
				debuglog("builtin write %s %d@%d\n", inode->name, dlen, doff);
				if (bn->write) {
					int l = bn->write(bn, (unsigned char *)wa->data.data_val, doff, dlen);
					res.status = (l == dlen) ? NFS_OK : NFSERR_IO;
				} else
					res.status = NFSERR_ACCES;
			}
			return &res;
		}
	}
	/* fetch attributes */
	if ((cp = search_cache(attrcache, inode->inode)) == 0) {
		gres = nfsproc_getattr_2((struct nfs_fh *) wa->file.data);
		if (gres->status != NFS_OK) {
			res.status = gres->status;
			return &res;
		}
		cp = search_cache(attrcache, inode->inode);
		if (!cp) {
			errorlog("nfsproc_write_2: cache is NULL\n");
			res.status = NFSERR_IO;
			return &res;
		}
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
				debuglog("Check: %d=%d,%d,%d>=%d\n",
				   inode->inode, cp->inode, dcp->towrite,
				       cp->actual_size, dcp->offset);
		for (dcp = cp->dcache; dcp; dcp = dcp->next)
			if (dcp->towrite && cp->actual_size >= dcp->offset)
				break;
		if (!dcp)	/* Can't write any blocks */
			break;

		debuglog("writing off: %d, len: %d, act: %d\n",
			       dcp->offset, dcp->len, cp->actual_size);

		debuglog("RFSV write %s\n", inode->name);

		if (rfsv_write(dcp->data, dcp->offset, dcp->len, inode->name) != dcp->len) {
			debuglog("write: dump failed\n");
			res.status = rfsv_isalive() ? NFSERR_NOSPC : NO_PSION;
			return &res;
		}
		dcp->towrite = 0;
		len = dcp->offset + dcp->len;
		if (len > cp->actual_size)
			cp->actual_size = len;
		debuglog("written: new length: %d\n", cp->actual_size);
	}

	debuglog("write -> ISOK (%d, %d %d)\n",
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
	debuglog("writecache???\n");
	res = (char) NFSERR_FBIG;
	return (void *) &res;
}

void *
nfsproc_null_2()
{
	static char res;
	debuglog("null.\n");
	res = (char) NFSERR_FBIG;
	return (void *) &res;
}

void *
nfsproc_root_2()
{
	static char res;
	debuglog("root????\n");
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

	debuglog("link..\n");
	res = NFSERR_ACCES;
	return &res;
}

/* ARGSUSED */
struct readlinkres *
nfsproc_readlink_2(struct nfs_fh *fh)
{
	static readlinkres res;

	debuglog("readlink...\n");
	res.status = NFSERR_ACCES;
	return &res;
}

/* ARGSUSED */
nfsstat *
nfsproc_symlink_2(symlinkargs *sa)
{
	static nfsstat res;

	debuglog("symlink..\n");
	res = NFSERR_ACCES;
	return &res;
}
