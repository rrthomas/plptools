INCLUDES=-I$(top_srcdir)/lib

sbin_PROGRAMS = plpfuse
plpfuse_LDADD = $(LIB_PLP) $(INTLLIBS) -lfuse -lattr
plpfuse_SOURCES = main.cc fuse.c

EXTRA_DIST = rfsv_api.h plpfuse.h
