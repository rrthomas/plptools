// $Id$
//
//  Copyright (C) 1999 Fritz Elfert <felfert@to.com>
//

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <syslog.h>

#include "defs.h"
#include "bool.h"
#include "rfsv.h"
#include "rfsvfactory.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"
extern "C" {
#include "rfsv_api.h"
}

static rfsv *a;
static rfsvfactory *rf;
static char *a_filename = 0;
static long  a_handle;
static long  a_offset;
static long  a_openmode;

long rfsv_isalive() {
	if (!a) {
		if (!(a = rf->create(true)))
			return 0;
	}
	return (a->getStatus() == 0);
}

long rfsv_dir(const char *file, dentry **e) {
	bufferArray entries;
	dentry *tmp;
	long ret;

	if (!a)
		return -1;
	ret = a->dir(&(*file), &entries);
	while (!entries.empty()) {
		bufferStore s;
		s = entries.pop();
		tmp = *e;
		*e = (dentry *)malloc(sizeof(dentry));
		if (!*e)
			return -1;
		(*e)->time = s.getDWord(0);
		(*e)->size = s.getDWord(4);
		(*e)->attr = a->attr2std(s.getDWord(8));
		(*e)->name = strdup(s.getString(12));
		(*e)->next = tmp;
	}
	return ret;
}

long rfsv_dircount(const char *file, long *count) {
	if (!a)
		return -1;
	return a->dircount(&(*file), &(*count));
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

static long rfsv_closecached() {
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

long rfsv_fcreate(long attr, const char *file, long *handle) {
	long ph;
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
	long ret = 0;

	if (!a)
		return -1;
	if (!a_filename || strcmp(a_filename, name) || a_openmode != rfsv::PSI_O_RDONLY) {
		rfsv_closecached();
		if((ret = rfsv_opencached(name, rfsv::PSI_O_RDONLY)))
			return ret;
	}
	if (a_offset != offset)
		ret = a->fseek(a_handle, offset, rfsv::PSI_SEEK_SET);
	if (ret >= 0) {
		a_offset = offset;
		ret = a->fread(a_handle, (unsigned char *)buf, len);
		if (ret <= 0)
			return ret;
		a_offset += ret;
	}
	return ret;
}

long rfsv_write(char *buf, long offset, long len, char *name) {
	long ret = 0;

	if (!a)
		return -1;

	if (!a_filename || strcmp(a_filename, name) || a_openmode != rfsv::PSI_O_RDWR) {
		if ((ret = rfsv_closecached()))
			return ret;
		if ((ret = rfsv_opencached(name, rfsv::PSI_O_RDWR)))
			return ret;
	}
	if (a_offset != offset)
		ret = a->fseek(a_handle, offset, rfsv::PSI_SEEK_SET);
	if (ret >= 0) {
		a_offset = offset;
		ret = a->fwrite(a_handle, (unsigned char *)buf, len);
		if (ret <= 0)
			return ret;
		a_offset += ret;
	}
	return ret;
}

long rfsv_setmtime(const char *name, long time) {
	if (!a)
		return -1;
	if (a_filename && !strcmp(a_filename, name))
		rfsv_closecached();
	return a->fsetmtime(name, time);
}

long rfsv_setsize(const char *name, long size) {
	long ph;
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
	dattr = a->std2attr(dattr);
	sattr = a->std2attr(sattr);
	return a->fsetattr(name, dattr, sattr);
}

long rfsv_getattr(const char *name, long *attr, long *size, long *time) {
	long res, psiattr;
	
	if (!a)
		return -1;
	res = a->fgeteattr(&(*name), &psiattr, &(*size), &(*time));
	*attr = a->attr2std(psiattr);
	return res;
}

long rfsv_statdev(char letter) {
	long vfree, vtotal, vattr, vuniqueid;
	int devnum = letter - 'A';
	char *name;

	if (!a)
		return -1;
	name = a->devinfo(devnum, &vfree, &vtotal, &vattr, &vuniqueid);
	return (name == NULL);
}

long rfsv_rename(const char *oldname, const char *newname) {
	if (!a)
		return -1;
	return a->rename(oldname, newname);
}

long rfsv_drivelist(int *cnt, device **dlist) {
	*dlist = NULL;
	long devbits;
	long ret;
	int i;

	if (!a)
		return -1;
	ret = a->devlist(&devbits);
	if (ret == 0)
		for (i = 0; i<26; i++) {
			char *name;
			long vtotal, vfree, vattr, vuniqueid;

			if ((devbits & 1) &&
			    ((name = a->devinfo(i, &vfree, &vtotal, &vattr, &vuniqueid)))) {
				device *next = *dlist;
				*dlist = (device *)malloc(sizeof(device));
				(*dlist)->next = next;
				(*dlist)->name = name;
				(*dlist)->total = vtotal;
				(*dlist)->free = vfree;
				(*dlist)->letter = 'A' + i;
				(*dlist)->attrib = vattr;
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
	char *user = 0L;
	char *mdir = DDIR;
	int sockNum = DPORT;
	int verbose = 0;
	int status = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc) {
			sockNum = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
			mdir = argv[++i];
		} else if (!strcmp(argv[i], "-u") && i + 1 < argc) {
			user = argv[++i];
		} else if (!strcmp(argv[i], "-v")) {
			verbose++;
		} else if (!strcmp(argv[i], "-V")) {
			cout << "plpnfsd version " << VERSION << endl;
			exit(0);
		} else
			usage();
	}
	signal(SIGPIPE, SIG_IGN);
	skt = new ppsocket();
	if (!skt->connect(NULL, sockNum)) {
		cerr << "plpnfsd: could not connect to ncpd" << endl;
		status = 1;
	} else {
		rf = new rfsvfactory(skt);
		a = rf->create(true);
		openlog("plpnfsd", LOG_PID|LOG_CONS, LOG_DAEMON);
		if (a != NULL)
			syslog(LOG_INFO, "connected, status is %d", status);
		else
			syslog(LOG_INFO, "could not create rfsv object, connect delayed");
		status = mp_main(verbose, mdir, user);
		delete a;
	}
	exit(status);
}
