AC_INIT([contactcards], [0.15], [https://github.com/florianl/ContactCards/issues])
AC_CONFIG_SRCDIR([src/contactcards.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC([gcc])
AM_PROG_CC_C_O

IT_PROG_INTLTOOL([0.40])
GETTEXT_PACKAGE=contactcards
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h locale.h stdlib.h stdio.h string.h])

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

PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.12 glib-2.0 gdk-3.0 gdk-pixbuf-2.0 gio-2.0 gobject-2.0 cairo, GTK3_SUFFIX=gtk3, AC_MSG_ERROR([gtk3 is required to compile ContactCards]))
AC_CHECK_LIB(gtk-3, gtk_main)
AC_CHECK_LIB(glib-2.0, g_free)
AC_CHECK_LIB(gdk-3, gdk_rgba_to_string)
AC_CHECK_LIB(gdk_pixbuf-2.0, gdk_pixbuf_new_from_stream)
AC_CHECK_LIB(gio-2.0, g_memory_input_stream_new_from_data)
AC_CHECK_LIB(gobject-2.0, g_object_set)
AC_CHECK_LIB(cairo, cairo_destroy)

PKG_CHECK_MODULES(NEON, neon, NEON_SUFFIX=neon, AC_MSG_ERROR([neon is required to compile ContactCards]))
AC_CHECK_LIB(neon, ne_session_create)

PKG_CHECK_MODULES(SQLITE3, sqlite3, SQLITE3_SUFFIX=sqlite3, AC_MSG_ERROR([sqlite3 is required to compile ContactCards]))
AC_CHECK_LIB(sqlite3, sqlite3_exec)

PKG_CHECK_MODULES(LDNS, ldns, AC_DEFINE(_USE_DANE), AC_CHECK_LIB(ldns, ldns_dane_verify, [LDNS_LIBS=-lldns; AC_DEFINE(_USE_DANE)], AC_MSG_ERROR([ldns is not found])))

if test _USE_DANE; then
	PKG_CHECK_MODULES(OPENSSL, openssl, OPENSSL_SUFFIX=openssl, AC_MSG_ERROR([openssl is required for ldns]))
	AC_CHECK_LIB(ssl, SSL_new)
	AC_CHECK_LIB(crypto, X509_STORE_new)
fi

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile po/Makefile.in])
AC_OUTPUT

echo "  C compiler    = $CC"
echo "  GTK flags     = $GTK3_CFLAGS"
echo "  GTK libs      = $GTK3_LIBS"
echo "  NEON flags    = $NEON_CFLAGS"
echo "  NEON libs     = $NEON_LIBS"
echo "  SQLITE3 flags = $SQLITE3_CFLAGS"
echo "  SQLITE3 libs  = $SQLITE3_LIBS"
echo "  LDNS flags    = $LDNS_CFLAGS"
echo "  LDNS libs     = $LDNS_LIBS"
if test _USE_DANE; then
	echo "  OPENSSL flags = $OPENSSL_CFLAGS"
	echo "  OPENSSL libs  = $OPENSSL_LIBS"
fi
