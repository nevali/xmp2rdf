## Copyright 2012 Mo McRoberts.
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.

AC_INIT([xmp2rdf],[master])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])

AC_PROG_CC
if test x"$GCC" = x"yes" ; then
   AM_CPPFLAGS="$AM_CPPFLAGS -W -Wall"
fi

old_CPPFLAGS="$CPPFLAGS"
old_LDFLAGS="$LDFLAGS"
AC_ARG_WITH([libxml2],[AS_HELP_STRING([--with-libxml2=PREFIX],[specify path to libxml2; libraries in PREFIX/lib, includes in PREFIX/include/libxml2])],[
  if test x"$withval" = x"yes" || test x"$withval" = x"no" ; then
    true
  else
    LIBXML2_CPPFLAGS="-I$withval/include/libxml2"
    LIBXML2_LDFLAGS="$LIBXML2_LDFLAGS -L$withval/lib"
  fi
])

CPPFLAGS="$CPPFLAGS $LIBXML2_CPPFLAGS"
LDFLAGS="$LDFLAGS $LIBXML2_LDFLAGS"
AC_CHECK_LIB([xml2],[xmlParseFile],,[AC_MSG_ERROR([libxml2 <http://xmlsoft.org/> is required to build xmp2rdf])])
AC_CHECK_HEADER([libxml/tree.h],,[AC_MSG_ERROR([cannot find libxml2's libxml/tree.h; please verify that
the development package for libxml2 is installed (if appropriate), and specify
--with-libxml2 or \$INCLUDES appropriately.])])
CPPFLAGS="$old_CPPFLAGS"
LDFLAGS="$old_LDFLAGS"
AM_CPPFLAGS="$AM_CPPFLAGS $LIBXML2_CPPFLAGS"
AM_LDFLAGS="$AM_LDFLAGS $LIBXML2_LDFLAGS"

PKG_CHECK_MODULES([REDLAND],[redland],,[AC_MSG_ERROR([Redland is required to build xmp2rdf])])

AM_CPPFLAGS="$AM_CPPFLAGS $REDLAND_CFLAGS"
LIBS="$REDLAND_LIBS $LIBS"

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST([LIBS])

AC_CHECK_HEADERS([unistd.h])

AC_CONFIG_FILES([Makefile
])

AC_OUTPUT
