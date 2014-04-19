AC_INIT([ContactCards], [0.08], [https://github.com/florianl/ContactCards/issues])
AC_CONFIG_SRCDIR([src/ContactCards.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_SILENT_RULES([yes])
AC_PROG_CC_C99

# Checks for programs.
AC_PROG_CC([gcc])
AM_PROG_CC_C_O

IT_PROG_INTLTOOL([0.40])
GETTEXT_PACKAGE=ContactCards
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h locale.h stdlib.h string.h sqlite3.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.10, GTK3_SUFFIX=gtk3, AC_MSG_ERROR([gtk3 is required to compile ContactCards]))
AC_SUBST(GTK3_CFLAGS)
AC_SUBST(GTK3_LIBS)

PKG_CHECK_MODULES(NEON, neon, NEON_SUFFIX=neon, AC_MSG_ERROR([neon is required to compile ContactCards]))
AC_SUBST(NEON_CFLAGS)
AC_SUBST(NEON_LIBS)

PKG_CHECK_MODULES(SQLITE3, sqlite3, SQLITE3_SUFFIX=sqlite3, AC_MSG_ERROR([sqlite3 is required to compile ContactCards]))
AC_SUBST(SQLITE3_CFLAGS)
AC_SUBST(SQLITE3_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile po/Makefile.in])
AC_OUTPUT
