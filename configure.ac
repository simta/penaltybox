AC_INIT([penaltybox], m4_esyscmd([bash version.sh]), [simta@umich.edu])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL

PKG_CHECK_MODULES([URCL], [urcl >= 1.1])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile packaging/rpm/penaltybox.spec])
AC_OUTPUT
