// $Id$
//
//  Copyright (C) 1999 Fritz Elfert <felfert@to.com>
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <syslog.h>

#include "rfsv.h"
#include "rpcs.h"
#include "rfsvfactory.h"
#include "rpcsfactory.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"
extern "C" {
#include "rfsv_api.h"
}

static rfsv *a;
static rfsvfactory *rf;
static char *a_filename = 0;
static u_int32_t a_handle;
static u_int32_t a_offset;
static u_int32_t a_openmode;

static rpcs *r;
static rpcsfactory *rp;
static bufferStore owner;

long rpcs_isalive() {
	long s;

	if (!r) {
		if (!(r = rp->create(true)))
			return 0;
	}
	s = r->getStatus();
	if (s == rfsv::E_PSI_FILE_DISC)
		r->reconnect();
	return (r->getStatus() == rfsv::E_PSI_GEN_NONE);
}

long rpcs_ownerRead(builtin_node *, char *buf, unsigned long  offset, long len) {

	if (offset >= (owner.getLen() - 1))
		return 0;
	const char *s = owner.getString(offset);
	int sl = strlen(s);
	if (sl > len)
		sl = len;
	strncpy(buf, s, sl);
	return sl;
}

long rpcs_ownerSize(builtin_node *) {
	Enum<rfsv::errs> res;
	bufferArray a;

	if (!rpcs_isalive())
		return 0;
	res = r->getOwnerInfo(a);
	owner.init();
	if (res == rfsv::E_PSI_GEN_NONE) {
		while (!a.empty()) {
			owner.addString(a.pop().getString());
			owner.addByte('\n');
		}
	}
	owner.addByte(0);
	return owner.getLen() - 1;
}

static long psread(builtin_node *node, char *buf, unsigned long offset, long len) {
	char *s = (char *)node->private_data;

	if (!s)
		return 0;
	if (offset >= ((unsigned long)node->size - 1))
		return 0;
	s += offset;
	int sl = node->size - offset;
	if (sl > len)
		sl = len;
	strncpy(buf, s, sl);
	return sl;
}

long rpcs_ps() {
	Enum<rfsv::errs> res;
	bufferArray psbuf;

	if (!rpcs_isalive())
		return -1;
	res = r->queryDrive('C', psbuf);
	if (res != rfsv::E_PSI_GEN_NONE)
		return -1;
	while (!psbuf.empty()) {
		builtin_node *dn;
		builtin_node *fn1;
		builtin_node *fn2;
		builtin_node *bn;
		char bname[40];

		bufferStore bs = psbuf.pop();
		bufferStore bs2 = psbuf.pop();
		sprintf(bname, "%d", bs.getWord(0));

		dn = (builtin_node *)malloc(sizeof(builtin_node));
		if (!dn)
			return -1;
		fn1 = (builtin_node *)malloc(sizeof(builtin_node));
		if (!fn1) {
			free(dn);
			return -1;
		}
		fn2 = (builtin_node *)malloc(sizeof(builtin_node));
		if (!fn2) {
			free(fn1);
			free(dn);
			return -1;
		}
		memset(dn, 0, sizeof(builtin_node));
		memset(fn1, 0, sizeof(builtin_node));
		memset(fn2, 0, sizeof(builtin_node));

		/**
		 * Directory, named by the PID
		 */
		dn->flags = BF_ISPROCESS;
		dn->name = bname;
		dn->attr = PSI_A_DIR;
		dn->getlinks = generic_getlinks;
		dn->getdents = generic_getdents;

		fn1->name = "cmd";
		fn1->attr = PSI_A_READ | PSI_A_RDONLY;
		fn1->private_data = (char *)malloc(strlen(bs.getString(2))+2);
		if (!fn1->private_data) {
			free(fn1);
			free(fn2);
			free(dn);
			return -1;
		}
		fn1->read = psread;
		sprintf(fn1->private_data, "%s\n", bs.getString(2));
		fn1->size = strlen(fn1->private_data);

		fn2->name = "args";
		fn2->attr = PSI_A_READ | PSI_A_RDONLY;
		fn2->private_data = (char *)malloc(strlen(bs2.getString())+2);
		if (!fn2->private_data) {
			free(fn1->private_data);
			free(fn1);
			free(fn2);
			free(dn);
			return -1;
		}
		fn2->read = psread;
		sprintf(fn2->private_data, "%s\n", bs2.getString());
		fn2->size = strlen(fn2->private_data);

		if (!(bn = register_builtin("proc", dn))) {
			free(fn1->private_data);
			free(fn1);
			free(fn2->private_data);
			free(fn2);
			free(dn);
			return -1;
		}
		strcpy(bname, builtin_path(bn));
		if (!register_builtin(bname, fn1)) {
			free(fn1->private_data);
			free(fn1);
			free(fn2->private_data);
			free(fn2);
			unregister_builtin(bn);
			free(dn);
			return -1;
		}
		if (!register_builtin(bname, fn2)) {
			free(fn2->private_data);
			free(fn2);
			unregister_builtin(bn);
			free(dn);
			return -1;
		}
		free(fn1);
		free(fn2);
		free(dn);
	}
	return 0;
}

long rfsv_isalive() {
	if (!a) {
		if (!(a = rf->create(true)))
			return 0;
	}
	return (a->getStatus() == rfsv::E_PSI_GEN_NONE);
}

long rfsv_dir(const char *file, dentry **e) {
	PlpDir entries;
	dentry *tmp;
	long ret;

	if (!a)
		return -1;
	ret = a->dir(file, entries);

	for (int i = 0; i < entries.size(); i++) {
		PlpDirent pe = entries[i];
		tmp = *e;
		*e = (dentry *)malloc(sizeof(dentry));
		if (!*e)
			return -1;
		(*e)->time = pe.getPsiTime().getTime();
		(*e)->size = pe.getSize();
		(*e)->attr = pe.getAttr();
		(*e)->name = strdup(pe.getName());
		(*e)->next = tmp;
	}
	return ret;
}

long rfsv_dircount(const char *file, u_int32_t *count) {
	if (!a)
		return -1;
	return a->dircount(file, *count);
}

long rfsv_rmdir(const char *name) {
	if (!a)
		return -1;
	return a->rmdir(name);
}

long rfsv_mkdir(const char *file) {
	if (!a)
		return -1;
	return a->mkdir(file);
}

long rfsv_closecached() {
	if (!a)
		return -1;
	if (!a_filename)
		return 0;
	a->fclose(a_handle);
	free(a_filename);
	a_filename = 0;
	return 0;
}

long rfsv_remove(const char *file) {
	if (!a)
		return -1;
	if (a_filename && !strcmp(a_filename, file))
		rfsv_closecached();
	return a->remove(file);
}

long rfsv_fclose(long handle) {
	if (!a)
		return -1;
	if (a_filename && (handle == a_handle)) {
		free(a_filename);
		a_filename = 0;
	}
	return a->fclose(handle);
}

long rfsv_fcreate(long attr, const char *file, u_int32_t *handle) {
	u_int32_t ph;
	long ret;

	if (!a)
		return -1;
	if (a_filename && !strcmp(a_filename, file))
		rfsv_closecached();
	ret = a->fcreatefile(attr, file, ph);
	*handle = ph;
	return ret;
}

static long rfsv_opencached(const char *name, long mode) {
	long ret;
	int retry = 100;

	if (!a)
		return -1;
	while (((ret = a->fopen(a->opMode(mode), name, a_handle))
		== rfsv::E_PSI_GEN_INUSE) && retry--)
		;
	if (ret)
		return ret;
	a_offset = 0;
	a_openmode = mode;
	a_filename = strdup(name);
	return ret;
}

long rfsv_read(char *buf, long offset, long len, char *name) {
	u_int32_t ret = 0;
	u_int32_t r_offset;

	if (!a)
		return -1;
	if (!a_filename || strcmp(a_filename, name) || a_openmode != rfsv::PSI_O_RDONLY) {
		rfsv_closecached();
		if((ret = rfsv_opencached(name, rfsv::PSI_O_RDONLY)))
			return ret;
	}
	if (a_offset != offset) {
		if (a->fseek(a_handle, offset, rfsv::PSI_SEEK_SET, r_offset) != rfsv::E_PSI_GEN_NONE)
			return -1;
		if (offset != r_offset)
			return -1;
	}
	a_offset = offset;
	if (a->fread(a_handle, (unsigned char *)buf, len, ret) != rfsv::E_PSI_GEN_NONE)
		return -1;
	a_offset += ret;
	return ret;
}

long rfsv_write(char *buf, long offset, long len, char *name) {
	u_int32_t ret = 0;
	u_int32_t r_offset;

	if (!a)
		return -1;

	if (!a_filename || strcmp(a_filename, name) || a_openmode != rfsv::PSI_O_RDWR) {
		if ((ret = rfsv_closecached()))
			return ret;
		if ((ret = rfsv_opencached(name, rfsv::PSI_O_RDWR)))
			return ret;
	}
	if (a_offset != offset) {
		if (a->fseek(a_handle, offset, rfsv::PSI_SEEK_SET, r_offset) != rfsv::E_PSI_GEN_NONE)
			return -1;
		if (offset != r_offset)
			return -1;
	}
	a_offset = offset;
	if (a->fwrite(a_handle, (unsigned char *)buf, len, ret) != rfsv::E_PSI_GEN_NONE)
		return -1;
	a_offset += ret;
	return ret;
}

long rfsv_setmtime(const char *name, long time) {
	if (!a)
		return -1;
	if (a_filename && !strcmp(a_filename, name))
		rfsv_closecached();
	return a->fsetmtime(name, PsiTime(time));
}

long rfsv_setsize(const char *name, long size) {
	u_int32_t ph;
	long ret;

	if (!a)
		return -1;
	if (a_filename && !strcmp(name, a_filename))
		return a->fsetsize(a_handle, size);
	ret = a->fopen(a->opMode(rfsv::PSI_O_RDWR), name, ph);
	if (!ret) {
		ret = a->fsetsize(ph, size);
		a->fclose(ph);
	}
	return ret;
}

long rfsv_setattr(const char *name, long sattr, long dattr) {
	if (!a)
		return -1;
	if (a_filename && !strcmp(name, a_filename))
		rfsv_closecached();
	return a->fsetattr(name, dattr, sattr);
}

long rfsv_getattr(const char *name, long *attr, long *size, long *time) {
	long res;
	PlpDirent e;

	if (!a)
		return -1;
	res = a->fgeteattr(name, e);
	*attr = e.getAttr();
	*size = e.getSize();
	*time = e.getPsiTime().getTime();
	return res;
}

long rfsv_statdev(char letter) {
	PlpDrive drive;

	if (!a)
		return -1;
	return (a->devinfo(letter, drive) != rfsv::E_PSI_GEN_NONE);
}

long rfsv_rename(const char *oldname, const char *newname) {
	if (!a)
		return -1;
	return a->rename(oldname, newname);
}

long rfsv_drivelist(int *cnt, device **dlist) {
	*dlist = NULL;
	u_int32_t devbits;
	long ret;
	int i;

	if (!a)
		return -1;
	ret = a->devlist(devbits);
	if (ret == 0)
		for (i = 0; i<26; i++) {
			PlpDrive drive;

			if ((devbits & 1) &&
			    ((a->devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE))) {

				device *next = *dlist;
				*dlist = (device *)malloc(sizeof(device));
				(*dlist)->next = next;
				(*dlist)->name = strdup(drive.getName().c_str());
				(*dlist)->total = drive.getSize();
				(*dlist)->free = drive.getSpace();
				(*dlist)->letter = 'A' + i;
				(*dlist)->attrib = drive.getMediaType();
				(*cnt)++;
			}
			devbits >>= 1;
		}
	return ret;
}

void usage()
{
	cerr << "usage: plpnfsd [-v] [-V] [-p port] [-d mountdir] [-u user]\n";
	exit(1);
}

int main(int argc, char**argv) {
	ppsocket *skt;
	ppsocket *skt2;
	char *user = 0L;
	char *mdir = DMOUNTPOINT;
	int sockNum = DPORT;
	int verbose = 0;
	int status = 0;

	struct servent *se = getservbyname("psion", "tcp");
	endservent();
	if (se != 0L)
		sockNum = ntohs(se->s_port);

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc) {
			sockNum = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
			mdir = argv[++i];
		} else if (!strcmp(argv[i], "-u") && i + 1 < argc) {
			user = argv[++i];
		} else if (!strcmp(argv[i], "-v")) {
			verbose++;
		} else if (!strcmp(argv[i], "-D")) {
			debug++;
		} else if (!strcmp(argv[i], "-V")) {
			cout << "plpnfsd version " << VERSION << endl;
			exit(0);
		} else
			usage();
	}

	skt = new ppsocket();
	if (!skt->connect(NULL, sockNum)) {
		cerr << "plpnfsd: could not connect to ncpd" << endl;
		status = 1;
	}
	skt2 = new ppsocket();
	if (!skt2->connect(NULL, sockNum)) {
		cerr << "plpnfsd: could not connect to ncpd" << endl;
		status = 1;
	}
	if (status == 0) {
		rf = new rfsvfactory(skt);
		rp = new rpcsfactory(skt2);
		a = rf->create(true);
		r = rp->create(true);
		openlog("plpnfsd", LOG_PID|LOG_CONS, LOG_DAEMON);
		if ((a != NULL) && (r != NULL))
			syslog(LOG_INFO, "connected, status is %d", status);
		else
			syslog(LOG_INFO, "could not create rfsv or rpcs object, connect delayed");
		status = mp_main(verbose, mdir, user);
		delete a;
		delete r;
	}
	exit(status);
}
