AM_CPPFLAGS=-I$(top_srcdir)/lib $(FUSE_CFLAGS)

sbin_PROGRAMS = plpfuse
plpfuse_LDADD = $(LIB_PLP) $(INTLLIBS) $(FUSE_LIBS) -lattr
plpfuse_SOURCES = main.cc fuse.c

EXTRA_DIST = rfsv_api.h plpfuse.h
