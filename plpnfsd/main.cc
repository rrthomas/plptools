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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

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
    processList psbuf;

    if (!rpcs_isalive())
	return -1;
    res = r->queryPrograms(psbuf);
    if (res != rfsv::E_PSI_GEN_NONE)
	return -1;
    processList::iterator i;
    for (i = psbuf.begin(); i != psbuf.end(); i++) {
	builtin_node *dn;
	builtin_node *fn1;
	builtin_node *fn2;
	builtin_node *bn;
	char bname[40];

	sprintf(bname, "%d", i->getPID());

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
	fn1->private_data = (char *)malloc(strlen(i->getName())+2);
	if (!fn1->private_data) {
	    free(fn1);
	    free(fn2);
	    free(dn);
	    return -1;
	}
	fn1->read = psread;
	sprintf(fn1->private_data, "%s\n", i->getName());
	fn1->size = strlen(fn1->private_data);

	fn2->name = "args";
	fn2->attr = PSI_A_READ | PSI_A_RDONLY;
	fn2->private_data = (char *)malloc(strlen(i->getArgs())+2);
	if (!fn2->private_data) {
	    free(fn1->private_data);
	    free(fn1);
	    free(fn2);
	    free(dn);
	    return -1;
	}
	fn2->read = psread;
	sprintf(fn2->private_data, "%s\n", i->getArgs());
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

static void
help()
{
    cout << _(
	"Usage: plpnfsd [OPTIONS]...\n"
	"\n"
	"Supported options:\n"
	"\n"
        " -d, --mountpoint=DIR    Specify DIR as mountpoint\n"
	" -u, --user=USER         Specify USER who owns mounted dir.\n"
	" -v, --verbose           Increase verbosity\n"
	" -D, --debug             Increase debug level\n"
	" -h, --help              Display this text.\n"
	" -V, --version           Print version and exit.\n"
	" -p, --port=[HOST:]PORT  Connect to port PORT on host HOST.\n"
	"                         Default for HOST is 127.0.0.1\n"
	"                         Default for PORT is "
	) << DPORT << "\n\n";
}

static void
usage() {
    cerr << _("Try `plpnfsd --help' for more information") << endl;
}

static struct option opts[] = {
    {"help",       no_argument,       0, 'h'},
    {"verbose",    no_argument,       0, 'v'},
    {"debug",      no_argument,       0, 'D'},
    {"version",    no_argument,       0, 'V'},
    {"port",       required_argument, 0, 'p'},
    {"user",       required_argument, 0, 'u'},
    {"mountpoint", required_argument, 0, 'd'},
    {NULL,       0,                 0,  0 }
};

static void
parse_destination(const char *arg, const char **host, int *port)
{
    if (!arg)
	return;
    // We don't want to modify argv, therefore copy it first ...
    char *argcpy = strdup(arg);
    char *pp = strchr(argcpy, ':');

    if (pp) {
	// host.domain:400
	// 10.0.0.1:400
	*pp ++= '\0';
	*host = argcpy;
    } else {
	// 400
	// host.domain
	// host
	// 10.0.0.1
	if (strchr(argcpy, '.') || !isdigit(argcpy[0])) {
	    *host = argcpy;
	    pp = 0L;
	} else
	    pp = argcpy;
    }
    if (pp)
	*port = atoi(pp);
}

int main(int argc, char**argv) {
    ppsocket *skt;
    ppsocket *skt2;
    char *user = 0L;
    char *mdir = DMOUNTPOINT;
    const char *host = "127.0.0.1";
    int sockNum = DPORT;
    int verbose = 0;
    int status = 0;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	sockNum = ntohs(se->s_port);

    while (1) {
	int c = getopt_long(argc, argv, "hvDVp:u:d:", opts, NULL);
	if (c == -1)
	    break;
	switch (c) {
	    case '?':
		usage();
		return -1;
	    case 'V':
		cout << _("plpnfsd Version ") << VERSION << endl;
		return 0;
	    case 'h':
		help();
		return 0;
	    case 'v':
		verbose++;
		break;
	    case 'D':
		debug++;
		break;
	    case 'd':
		mdir = optarg;
		break;
	    case 'u':
		user = optarg;
		break;
	    case 'p':
		parse_destination(optarg, &host, &sockNum);
		break;
	}
    }
    if (optind < argc) {
	usage();
	return -1;
    }

    skt = new ppsocket();
    if (!skt->connect(host, sockNum)) {
	cerr << "plpnfsd: could not connect to ncpd" << endl;
	status = 1;
    }
    skt2 = new ppsocket();
    if (!skt2->connect(host, sockNum)) {
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

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
