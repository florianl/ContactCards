AC_INIT([contactcards], [0.28], [https://github.com/florianl/ContactCards/issues])
AC_CONFIG_SRCDIR([src/contactcards.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC([gcc])
AM_PROG_CC_C_O
AC_C_INLINE
AC_TYPE_UINT16_T

IT_PROG_INTLTOOL([0.40])
GETTEXT_PACKAGE=contactcards
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h locale.h stdlib.h stdio.h string.h])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([setlocale])
AC_CHECK_FUNCS([socket])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([sys/socket.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debugging, default: no]),
	[case "${enableval}" in
		yes) debug=true ;;
		no)  debug=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	esac], [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF(DEBUG, AC_MSG_NOTICE([Debugging enabled]),AC_MSG_NOTICE([Debugging disabled]))


AC_ARG_ENABLE(dane, AS_HELP_STRING([--enable-dane], [enable DANE, default: no]),
	[case "${enableval}" in
		yes) dane=true ;;
		no)  dane=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-dane]) ;;
	esac], [dane=false])
AM_CONDITIONAL(DANE, test x"$dane" = x"true")
AM_COND_IF(DANE, AC_MSG_NOTICE([DANE enabled]),AC_MSG_NOTICE([DANE disabled]))

PKG_CHECK_MODULES(GLIB, [glib-2.0], GLIB_SUFFIX=glib, AC_MSG_ERROR([glib-2.0 is required to compile ContactCards]))

PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.12 gdk-3.0 gdk-pixbuf-2.0 gio-2.0 gobject-2.0 cairo, GTK3_SUFFIX=gtk3, AC_MSG_ERROR([gtk3 is required to compile ContactCards]))

PKG_CHECK_MODULES(NEON, neon, NEON_SUFFIX=neon, AC_MSG_ERROR([neon is required to compile ContactCards]))

PKG_CHECK_MODULES(SQLITE3, sqlite3, SQLITE3_SUFFIX=sqlite3, AC_MSG_ERROR([sqlite3 is required to compile ContactCards]))

if test "$dane" = "true"; then
	PKG_CHECK_MODULES(LDNS, ldns, AC_DEFINE(_USE_DANE), AC_CHECK_LIB(ldns, ldns_dane_verify, [LDNS_LIBS=-lldns; AC_DEFINE(_USE_DANE)], AC_MSG_ERROR([ldns is not found])))
	PKG_CHECK_MODULES(OPENSSL, openssl, OPENSSL_SUFFIX=openssl, AC_MSG_ERROR([openssl is required for ldns]))
fi

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile po/Makefile.in])
AC_OUTPUT

echo "  C compiler    = $CC"
echo "  GTK flags     = $GTK3_CFLAGS"
echo "  GTK libs      = $GTK3_LIBS"
echo "  GLIB flags    = $GLIB_CFLAGS"
echo "  GLIB libs     = $GLIB_LIBS"
echo "  NEON flags    = $NEON_CFLAGS"
echo "  NEON libs     = $NEON_LIBS"
echo "  SQLITE3 flags = $SQLITE3_CFLAGS"
echo "  SQLITE3 libs  = $SQLITE3_LIBS"
if test "$dane" = "true"; then
	echo "  LDNS flags    = $LDNS_CFLAGS"
	echo "  LDNS libs     = $LDNS_LIBS"
	echo "  OPENSSL flags = $OPENSSL_CFLAGS"
	echo "  OPENSSL libs  = $OPENSSL_LIBS"
fi
