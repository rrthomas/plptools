/* $Id$
 *
 * Original version of this file from p3nfsd-5.4 by
 * Rudolf Koenig (rfkoenig@immd4.informatik.uni-erlangen.de)
 *
 * Modifications for plputils by Fritz Elfert <felfert@to.com>
 *
 */
#include <stdio.h>
#include "nfs_prot.h"
#include "mp.h"
#include "rfsv_api.h"

#if defined(__SVR4) || defined(__GLIBC__) || defined(__FreeBSD__)
#include <string.h>
#include <stdlib.h>
#endif
#ifdef __NeXT__
#include <string.h>
#include <objc/hashtable.h>
#define strdup NXCopyStringBuffer
#endif
#define HASHSIZE 999

static int nextinode = 6;
static p_inode *numtab[HASHSIZE];
static p_inode *namtab[HASHSIZE];

/*
 * Verrry simple hash :-)
 */
static unsigned
hash(str)
char *str;
{
	unsigned i = 0, hashval = 3 * HASHSIZE / 4;

	while (*str) {
		i = *str++;
		hashval = (hashval << (i & 7)) + i;
	}

	return hashval % HASHSIZE;
}

/* Get struct with inode */
p_inode *
get_num(i)
int i;
{
	p_inode *ptr;

	for (ptr = numtab[i % HASHSIZE]; ptr; ptr = ptr->nextnum)
		if (i == ptr->inode)
			break;
	if (!ptr) {
		errorlog("Inode %d not found (aborting)\n", i);
		abort();
	}
	return ptr;
}

static p_inode *
newinode(name, inode)
char *name;
int inode;
{
	p_inode *ptr;
	int idx = hash(name);

	ptr = (p_inode *) malloc(sizeof(*ptr));
	ptr->name = (char *) strdup(name);
	ptr->inode = inode;

/* insert into both hashtabs */
	ptr->nextnam = namtab[idx];
	namtab[idx] = ptr;

	ptr->nextnum = numtab[inode % HASHSIZE];
	numtab[inode % HASHSIZE] = ptr;

	return ptr;
}

/* Get/create struct with name */
p_inode *
get_nam(name)
char *name;
{
	p_inode *ptr;
	int idx = hash(name);

	for (ptr = namtab[idx]; ptr; ptr = ptr->nextnam)
		if (!strcmp(name, ptr->name))
			break;
	if (!ptr)
		ptr = newinode(name, nextinode++);
	if (debug > 1)
		debuglog("get_nam(``%s'') returns %08x->inode = %d\n",
		       name, (unsigned int) ptr, ptr->inode);
	return ptr;
}

void
inode2fh(inode, fh)
int inode;
char *fh;
{
	bzero(fh, NFS_FHSIZE);
	bcopy((char *) &inode, fh, sizeof(inode));
}

int
fh2inode(fh)
char *fh;
{
	int inode;

	bcopy(fh, (char *) &inode, sizeof(inode));
	return inode;
}



/* Rename: the inode must be preserved */
p_inode *
re_nam(old, new)
char *old, *new;
{
	p_inode *nptr, *optr, **nampp, **numpp;
	int idx = hash(old);

	if (debug)
		debuglog("re_nam: %s->%s\n", old, new);
	for (nampp = &namtab[idx]; *nampp; nampp = &(*nampp)->nextnam)
		if (!strcmp(old, (*nampp)->name))
			break;
	if (!*nampp)
		return get_nam(new);

	optr = *nampp;
	if (debug)
		debuglog("re_nam: %d\n", optr->inode);
	*nampp = optr->nextnam;

	/* delete it from the other hashtab too */
	idx = optr->inode % HASHSIZE;
	for (numpp = &numtab[idx]; *numpp; numpp = &(*numpp)->nextnum)
		if (optr == (*numpp))
			break;
	if (!*numpp) {
		errorlog("Entry in one hashtab only (aborting)\n");
		abort();
	}
	*numpp = optr->nextnum;

	nptr = newinode(new, optr->inode);
	if (debug)
		debuglog("re_nam: new entry created\n");
	free(optr->name);
	free(optr);

	return nptr;
}

/* Cache routines */
struct cache *
search_cache(root, inode)
struct cache *root;
unsigned inode;
{
	struct cache *cp;

	if (debug)
		debuglog("search_cache %d\n", inode);
	for (cp = root; cp; cp = cp->next)
		if (cp->inode == inode) {
			cp->stamp = time(0);
			return cp;
		}
	return 0;
}

struct cache *
add_cache(struct cache **root, unsigned int inode, fattr *fp) {
	struct cache *cp;

	if (debug)
		debuglog("add_cache %d\n", inode);
	cp = (struct cache *) malloc(sizeof(*cp));
	if (cp != NULL) {
		cp->stamp = time(0);
		cp->inode = inode;
		cp->attr = *fp;
		cp->dcache = 0;
		cp->actual_size = fp->size;
		cp->next = *root;
		*root = cp;
	}
	return cp;
}

struct dcache *
add_dcache(cp, offset, len, data)
struct cache *cp;
unsigned offset, len;
unsigned char *data;
{
	struct dcache *dcp;
	dcp = (struct dcache *) malloc(sizeof(*dcp));
	dcp->towrite = 1;
	dcp->offset = offset;
	dcp->data = 0;
	dcp->len = len;
	if (len) {
		dcp->data = (unsigned char *) malloc(len);
		bcopy(data, dcp->data, len);
	}
	dcp->next = cp->dcache;
	cp->dcache = dcp;
	return dcp;
}

void
clean_dcache(cp)
struct cache *cp;
{
	struct dcache *dcp, *dcpn;
	for (dcp = cp->dcache; dcp; dcp = dcpn) {
		dcpn = dcp->next;
		if (dcp->len)
			free(dcp->data);
		free(dcp);
	}
	cp->dcache = 0;
}

struct dcache *
search_dcache(cp, off, len)
struct cache *cp;
unsigned int off, len;
{
	struct dcache *dcp;
	for (dcp = cp->dcache; dcp; dcp = dcp->next)
		if (dcp->offset == off && dcp->len >= len)
			return dcp;
	return 0;
}

void
rem_cache(struct cache **root, unsigned int inode) {
	struct cache *cp, **cpp;

	if (debug)
		debuglog("rem_cache %d\n", inode);
	for (cpp = root; (cp = *cpp); cpp = &cp->next)
		if (cp->inode == inode)
			break;
	if (!cp)
		return;
	*cpp = cp->next;
	clean_dcache(cp);
	free(cp);
}

time_t cache_keep = 30;

void
clean_cache(struct cache **root) {
	struct cache **cp = root;
	time_t now = time(0);

	while (*cp) {
		if (force_cache_clean || ((now - (*cp)->stamp) > cache_keep)) {
			struct cache *old = *cp;
			if (debug)
				debuglog("clean_cache %d\n", (*cp)->inode);
			*cp = (*cp)->next;
			clean_dcache(old);
			free(old);
		} else
			cp = &(*cp)->next;
	}
}

char *
build_path(dir, file)
char *dir, *file;
{
	static char namebuf[300];

	if (!strcmp(dir, ""))
		strcpy(namebuf, file);
	else
		sprintf(namebuf, "%s\\%s", dir, file);

	return namebuf;
}

int
getpinode(inode)
p_inode *inode;
{
	char *p;
	int i;

	if (inode->inode == root_fattr.fileid)	/* Root inode */
		i = root_fattr.fileid - 1;	/* RUDI !!! */
	else if (!(p = (char *) rindex(inode->name, '\\')))	/* device inode */
		i = root_fattr.fileid;
	else {
		*p = 0;
		i = get_nam(inode->name)->inode;
		*p = '\\';
	}
	return i;
}

char *
dirname(dir)
char *dir;
{
	static char namebuf[300];
	sprintf(namebuf, "%s\\", dir);
	return namebuf;
}

char *
filname(dir)
char *dir;
{
	char *p;
	if ((p = (char *) rindex(dir, '\\')))
		return p + 1;
	else
		return dir;
}
