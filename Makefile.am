SUBDIRS = . test

AM_CFLAGS = -Wall $(URCL_CFLAGS)
LDADD = $(URCL_LIBS)
COMMON_FILES = penaltybox.h yasl.h yasl.c

bin_PROGRAMS = penaltybox userthrottle ipthrottle

penaltybox_SOURCES = penaltybox.c $(COMMON_FILES)

userthrottle_SOURCES = userthrottle.c $(COMMON_FILES)

ipthrottle_SOURCES = ipthrottle.c $(COMMON_FILES)

EXTRA_DIST = VERSION packaging/rpm/penaltybox.spec

rpm: dist-xz
	rpmbuild -ta $(distdir).tar.xz
