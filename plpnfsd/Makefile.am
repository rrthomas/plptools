INCLUDES=-I../lib

LIBS = -L../lib -lutils

sbin_PROGRAMS = plpnfsd

plpnfsd_SOURCES = mp_main.c mp_mount.c nfs_prot_svc.c nfs_prot_xdr.c \
	mp_pfs_ops.c mp_inode.c main.cc

EXTRA_plpnfsd_SOURCES = mount_aix.c
