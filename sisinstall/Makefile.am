AM_CPPFLAGS=-I$(top_srcdir)/lib -I$(top_srcdir)/intl

bin_PROGRAMS = sisinstall
sisinstall_LDADD = ../lib/libplp.la $(INTLLIBS)
sisinstall_SOURCES = psion.cpp sisinstaller.cpp sismain.cpp \
	fakepsion.cpp sisfilelink.cpp sisfilelink.h
EXTRA_DIST = psion.h sisinstaller.h fakepsion.h
