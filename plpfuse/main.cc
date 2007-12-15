/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2007 Reuben Thomas <rrt@sc3d.org>
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

#include <rfsv.h>
#include <rpcs.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferstore.h>
#include <bufferarray.h>
#include <ppsocket.h>

#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "rfsv_api.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#include <fuse/fuse_lowlevel.h>

using namespace std;

static rfsv *a;
static rfsvfactory *rf;

static rpcs *r;
static rpcsfactory *rp;
static bufferStore owner;

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
	*e = (dentry *)calloc(1, sizeof(dentry));
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

long rfsv_remove(const char *file) {
    if (!a)
	return -1;
    return a->remove(file);
}

long rfsv_fclose(long handle) {
    if (!a)
	return -1;
    return a->fclose(handle);
}

long rfsv_fcreate(long attr, const char *file, u_int32_t *handle) {
    u_int32_t ph;
    long ret;

    if (!a)
	return -1;
    ret = a->fcreatefile(attr, file, ph);
    *handle = ph;
    return ret;
}

long rfsv_open(const char *name, long mode, u_int32_t *handle) {
    long ret, retry;

    if (!a)
	return -1;
    if (mode == O_RDONLY)
        mode = rfsv::PSI_O_RDONLY;
    else
        mode = rfsv::PSI_O_RDWR;
    for (retry = 100; retry > 0 && (ret = a->fopen(a->opMode(mode), name, *handle)) != rfsv::E_PSI_GEN_NONE; retry--)
        usleep(20000);
    return ret;
}

long rfsv_read(char *buf, long offset, long len, const char *name) {
    u_int32_t ret = 0, r_offset, handle;

    if (!a)
	return -1;
    if ((ret = rfsv_open(name, O_RDONLY, &handle)))
        return ret;
    if (a->fseek(handle, offset, rfsv::PSI_SEEK_SET, r_offset) != rfsv::E_PSI_GEN_NONE ||
        offset != r_offset ||
        a->fread(handle, (unsigned char *)buf, len, ret) != rfsv::E_PSI_GEN_NONE)
	ret = -1;
    rfsv_fclose(handle);
    return ret;
}

long rfsv_write(const char *buf, long offset, long len, const char *name) {
    u_int32_t ret = 0, r_offset, handle;

    if (!a)
	return -1;
    if ((ret = rfsv_open(name, O_RDWR, &handle)))
        return ret;
    if (a->fseek(handle, offset, rfsv::PSI_SEEK_SET, r_offset) != rfsv::E_PSI_GEN_NONE ||
        offset != r_offset ||
        a->fwrite(handle, (unsigned char *)buf, len, ret) != rfsv::E_PSI_GEN_NONE)
	ret = -1;
    rfsv_fclose(handle);
    return ret;
}

long rfsv_setmtime(const char *name, long time) {
    if (!a)
	return -1;
    return a->fsetmtime(name, PsiTime(time));
}

long rfsv_setsize(const char *name, long size) {
    u_int32_t ph;
    long ret;

    if (!a)
	return -1;
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
    return a->fsetattr(name, sattr, dattr);
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
	for (i = 0; i < 26; i++) {
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
    cerr << _(
	"Usage: plpfuse [OPTION...] MOUNTPOINT\n"
	"\n"
	"Supported options:\n"
	"\n"
	" -u, --user=USER         Specify USER who owns mounted dir\n"
	" -d, --debug             Increase debugging\n"
	" -h, --help              Display this text\n"
	" -V, --version           Print version and exit\n"
	" -p, --port=[HOST:]PORT  Connect to port PORT on host HOST\n"
	"                         Default for HOST is 127.0.0.1\n"
	"                         Default for PORT is "
	) << DPORT << "\n\n";
}

static struct option opts[] = {
    {"help",       no_argument,       0, 'h'},
    {"debug",      no_argument,       0, 'd'},
    {"version",    no_argument,       0, 'V'},
    {"port",       required_argument, 0, 'p'},
    {"user",       required_argument, 0, 'u'},
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

int fuse(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_chan *ch;
    char *mountpoint;
    int err = -1;

    if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 &&
        (ch = fuse_mount(mountpoint, &args)) != NULL) {
        struct fuse *fp = fuse_new(ch, &args, &plp_oper, sizeof(plp_oper), NULL);
        if (fp != NULL)
            err = fuse_loop(fp);
        fuse_unmount(mountpoint, ch);
    }
    fuse_opt_free_args(&args);

    return err ? 1 : 0;
}

int main(int argc, char**argv) {
    ppsocket *skt;
    ppsocket *skt2;
    const char *host = "127.0.0.1";
    int sockNum = DPORT;
    int i, c;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	sockNum = ntohs(se->s_port);

    opterr = 0; // Suppress errors from unknown options
    while ((c = getopt_long(argc, argv, "hVp:d", opts, NULL)) != -1) {
	switch (c) {
        case 'V':
            cerr << _("plpfuse version ") << VERSION << endl;
            break;
        case 'h':
            help();
            break;
        case 'd':
            debug++;
            break;
        case 'p':
            parse_destination(optarg, &host, &sockNum);
            for (i = optind; i < argc - 1; i++)
              argv[i] = argv[i + 2];
            argc -= 2;
            break;
	}
    }

    skt = new ppsocket();
    if (!skt->connect(host, sockNum)) {
        cerr << _("plpfuse: could not connect to ncpd") << endl;
	return 1;
    }
    skt2 = new ppsocket();
    if (!skt2->connect(host, sockNum)) {
        cerr << _("plpfuse: could not connect to ncpd") << endl;
        return 1;
    }

    rf = new rfsvfactory(skt);
    rp = new rpcsfactory(skt2);
    a = rf->create(true);
    r = rp->create(true);
    if (a != NULL && r != NULL)
        debuglog("plpfuse: connected");
    else
        debuglog("plpfuse: could not create rfsv or rpcs object, connect delayed");
    return fuse(argc, argv);
}
