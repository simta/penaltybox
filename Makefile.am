AM_CFLAGS = -Wall $(URCL_CFLAGS)
LDADD = $(URCL_LIBS)
COMMON_FILES = penaltybox.h yasl.h yasl.c

bin_PROGRAMS = penaltybox userthrottle

penaltybox_SOURCES = penaltybox.c $(COMMON_FILES)

userthrottle_SOURCES = userthrottle.c $(COMMON_FILES)

EXTRA_DIST = VERSION packaging/rpm/penaltybox.spec

rpm: dist-gzip
	rpmbuild -ta $(distdir).tar.gz
