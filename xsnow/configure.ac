#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021 Willem Vermin
#-# 
#-# This program is free software: you can redistribute it and/or modify
#-# it under the terms of the GNU General Public License as published by
#-# the Free Software Foundation, either version 3 of the License, or
#-# (at your option) any later version.
#-# 
#-# This program is distributed in the hope that it will be useful,
#-# but WITHOUT ANY WARRANTY; without even the implied warranty of
#-# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#-# GNU General Public License for more details.
#-# 
#-# You should have received a copy of the GNU General Public License
#-# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-# 

AC_PREREQ([2.69])
AC_INIT([xsnow], [3.3.2], [wvermin@gmail.com])
AC_CONFIG_SRCDIR([src/xsnow.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
./prevent-remakes
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC([cc gcc])
AC_PROG_CXX([c++ g++])
AC_PROG_INSTALL

LIBS="-lm"
# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([assert.h ctype.h math.h pthread.h signal.h],[],AC_MSG_ERROR([Exiting]))
AC_CHECK_HEADERS([stdarg.h stdio.h stdlib.h string.h unistd.h],[],AC_MSG_ERROR([Exiting]))
AC_CHECK_HEADERS([execinfo.h])

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([iostream],[],AC_MSG_ERROR([Check your C++ compiler. Exiting]))
AC_CHECK_HEADERS([unordered_map])
AC_CHECK_HEADERS([unordered_set])
AC_LANG_POP([C++])

PKG_CHECK_MODULES(XML, [libxml-2.0])
PKG_CHECK_MODULES(GTK, [gtk+-3.0 gmodule-2.0])
PKG_CHECK_MODULES(X11, [x11 xpm xt xext xproto])

AC_CHECK_FUNCS([backtrace])
LIBS="$X11_LIBS"
# check for availability of double buffering
AC_CHECK_FUNCS([XdbeAllocateBackBufferName])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
# check for AC_FUNC_MALLOC has detrimental effects on some systems
#  symptom: undefined reference to rpl_malloc
#  see also: https://github.com/maxmind/libmaxminddb/pull/152
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
#AC_FUNC_ALLOCA
AC_CHECK_FUNCS([alarm gettimeofday sqrt strchr strdup strstr strtol])

LIBS="-lm"

AC_CONFIG_FILES([Makefile src/Makefile] src/Pixmaps/Makefile)

AC_OUTPUT
