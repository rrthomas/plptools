//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  e-mail philip.proudman@btinternet.com

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "defs.h"
#include "bool.h"
#include "rfsv32.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"
extern "C" {
#include "rfsv_api.h"
}

static rfsv32 *a;

long rfsv_isalive() {
	return (a->getStatus() == 0);
}

long rfsv_dir(const char *file, dentry **e) {
	bufferArray entries;
	dentry *tmp;
	long ret = a->dir(&(*file), &entries);
	while (!entries.empty()) {
		bufferStore s;
		s = entries.popBuffer();
		tmp = *e;
		*e = (dentry *)malloc(sizeof(dentry));
		if (!*e)
			return -1;
		(*e)->time = s.getDWord(0);
		(*e)->size = s.getDWord(4);
		(*e)->attr = s.getDWord(8);
		(*e)->name = strdup(s.getString(12));
		(*e)->next = tmp;
	}
	return ret;
}

long rfsv_dircount(const char *file, long *count) {
	return a->dircount(&(*file), &(*count));
}

long rfsv_rmdir(const char *name) {
	return a->rmdir(name);
}

long rfsv_mkdir(const char *file) {
	if (!a)
		return -1;
	return a->mkdir(file);
}

long rfsv_remove(const char *file) {
	return a->remove(file);
}

long rfsv_fclose(long handle) {
	return a->fclose(handle);
}

long rfsv_fopen(long attr, const char *file, long *handle) {
	long ph;
	long ret = a->fopen(attr, file, ph);
	*handle = ph;
	return ret;
}

long rfsv_fcreate(long attr, const char *file, long *handle) {
	long ph;
	long ret = a->fcreatefile(attr, file, ph);
	*handle = ph;
	return ret;
}

long rfsv_read(char *buf, long offset, long len, long handle) {
	long ret = a->fseek(handle, offset, rfsv32::PSI_SEEK_SET);
	if (ret >= 0)
		ret = a->fread(handle, buf, len);
	return ret;
}

long rfsv_write(char *buf, long offset, long len, long handle) {
	long ret = a->fseek(handle, offset, rfsv32::PSI_SEEK_SET);
	if (ret >= 0)
		ret = a->fwrite(handle, buf, len);
	return ret;
}

long rfsv_setmtime(const char *name, long time) {
	return a->fsetmtime(name, time);
}

long rfsv_setsize(const char *name, long size) {
	long ph;
	long ret = a->fopen(rfsv32::PSI_OMODE_READ_WRITE, name, ph);
	if (!ret) {
		ret = a->fsetsize(ph, size);
		a->fclose(ph);
	}
	return ret;
}

long rfsv_setattr(const char *name, long sattr, long dattr) {
	return a->fsetattr(name, dattr, sattr);
}

long rfsv_getattr(const char *name, long *attr, long *size, long *time) {
	return a->fgeteattr(&(*name), &(*attr), &(*size), &(*time));
}

long rfsv_statdev(char letter) {
	long vfree, vtotal, vattr, vuniqueid;
	int devnum = letter - 'A';
	char *name;

    	name = a->devinfo(devnum, &vfree, &vtotal, &vattr, &vuniqueid);
	return (name == NULL);
}

long rfsv_rename(const char *oldname, const char *newname) {
	return a->rename(oldname, newname);
}

long rfsv_drivelist(int *cnt, device **dlist) {
	*dlist = NULL;
	long devbits;
	long ret;
	int i;

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
	skt->connect(NULL, sockNum);
	a = new rfsv32(skt);
	return mp_main(verbose, mdir, user);
}
