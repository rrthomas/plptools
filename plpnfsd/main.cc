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

#include "bool.h"
#include "../defaults.h"
#include "rfsv32.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"
extern "C" {
#include "rfsv_api.h"
}

static rfsv32 *a;

long rfsv_dir(const char *file, dentry **e) {
	bufferArray entries;
	dentry *tmp;
	long ret = a->dir(&(*file), &entries);
	psion_alive = (a->getStatus() == 0);
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
	long ret = a->dircount(&(*file), &(*count));
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_rmdir(const char *name) {
	long ret = a->rmdir(name);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_mkdir(const char *file) {
	long ret = a->mkdir(file);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_remove(const char *file) {
	long ret = a->remove(file);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_fclose(long handle) {
	long ret = a->fclose(handle);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_fopen(long attr, const char *file, long *handle) {
	long ph;
	long ret = a->fopen(attr, file, ph);
	*handle = ph;
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_fcreate(long attr, const char *file, long *handle) {
	long ph;
	long ret = a->fcreatefile(attr, file, ph);
	*handle = ph;
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_read(char *buf, long offset, long len, long handle) {
	long ret = a->fseek(handle, offset, rfsv32::PSEEK_SET);
	if (ret >= 0)
		ret = a->fread(handle, buf, len);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_write(char *buf, long offset, long len, long handle) {
	long ret = a->fseek(handle, offset, rfsv32::PSEEK_SET);
	if (ret >= 0)
		ret = a->fwrite(handle, buf, len);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_setmtime(const char *name, long time) {
	long ret = a->fsetmtime(name, time);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_setsize(const char *name, long size) {
	long ph;
	long ret = a->fopen(0x200, name, ph);
	if (!ret) {
		ret = a->fsetsize(ph, size);
		a->fclose(ph);
	}
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_setattr(const char *name, long sattr, long dattr) {
	long ret = a->fsetattr(name, dattr, sattr);
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_getattr(const char *name, long *attr, long *size, long *time) {
	long ret = a->fgeteattr(&(*name), &(*attr), &(*size), &(*time));
	psion_alive = (a->getStatus() == 0);
	return ret;
}

long rfsv_statdev(char letter) {
	long vfree, vtotal, vattr, vuniqueid;
	int devnum = letter - 'A';
	char *name;

    	name = a->devinfo(devnum, &vfree, &vtotal, &vattr, &vuniqueid);
	psion_alive = (a->getStatus() == 0);
	return (name == NULL);
}

long rfsv_rename(const char *oldname, const char *newname) {
	long ret = a->rename(oldname, newname);
	psion_alive = (a->getStatus() == 0);
	return ret;
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
	psion_alive = (a->getStatus() == 0);
	return ret;
}

int main(int argc, char**argv) {
  ppsocket *skt;
  bool res;
  
  // Command line parameter processing
  int sockNum = DEFAULT_SOCKET;

  skt = new ppsocket();
  skt->startup();
  res = skt->connect(NULL, sockNum);
  
  a = new rfsv32(skt);

  char *mp_args[] = { "mp_main", "-v", "-dir", "/mnt/psion", NULL };
  mp_main(4, mp_args);
  
  delete a;
  return 0;
}
