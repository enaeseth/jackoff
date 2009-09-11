AC_PREREQ(2.57)

AC_INIT([Jackoff], [0.1], [Eric Naeseth<enaeseth@gmail.com>], [jackoff]
	[http://github.com/enaeseth/jackoff])

AC_CONFIG_SRCDIR([src/jackoff.c])
AC_CANONICAL_SYSTEM

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE([1.7])
AM_CONFIG_HEADER([src/config.h])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_C_INLINE

# Check for JACK (need 0.100.0 for jack_client_open)
PKG_CHECK_MODULES(JACK, jack >= 0.100.0)

# Check for libsndfile
PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.18,
	[ HAVE_SNDFILE="Yes"
	  AC_DEFINE(HAVE_SNDFILE, 1, [SndFile library is available])
	], 
	[ HAVE_SNDFILE="No" 
	  AC_MSG_ERROR(Can't find libsndfile.)
	]
)

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])
AC_CHECK_FUNCS( usleep )

CFLAGS="$CFLAGS -Wunused -Wall $JACK_CFLAGS $SNDFILE_CFLAGS"
LDFLAGS="$LDFLAGS $JACK_LIBS $TWOLAME_LIBS $LAME_LIBS $SNDFILE_LIBS"

AC_OUTPUT([Makefile src/Makefile])