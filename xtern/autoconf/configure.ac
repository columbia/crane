dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([[[TERN]]],[[[0.10]]],[junfeng@cs.columbia.edu])

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.config.in"])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.config)

dnl Configure a header file
AC_CONFIG_HEADERS([include/tern/config.h])

dnl FIXME: Make out of tree builds work.

AC_PROG_CPP
AC_PROG_CC(gcc)
AC_PROG_CXX(g++)

AC_MSG_RESULT([$src_root])

AC_SUBST(SRC_ROOT,$src_root)

dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************

dnl Do special configuration of Makefiles

AC_DEFUN([AC_CONFIG_MAKEFILE],
[AC_CONFIG_COMMANDS($1,
  [${srcdir}/autoconf/mkinstalldirs `dirname $1`
   ${SHELL} ${srcdir}/autoconf/install-sh -m 0644 -c ${srcdir}/$1 $1])
])
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(Makefile.common)
AC_CONFIG_MAKEFILE(test/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile.tests)

dnl This must be last
AC_OUTPUT
