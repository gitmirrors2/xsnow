#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019 Willem Vermin
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
AC_INIT([xsnow], [2.0.16~pre2], [wvermin@gmail.com])
AC_CONFIG_SRCDIR([src/xsnow.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
./prevent-remakes
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

AC_CHECK_LIB([X11], [XOpenDisplay], [],[
	     echo "libX11 is required"
	     exit -1])
AC_CHECK_LIB([Xpm], [XpmCreatePixmapFromData], [], AC_MSG_ERROR([Exiting]))

AC_CHECK_LIB([m], [sin], [], [
	      echo "libm is required"
	      exit -1])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h math.h],[],AC_MSG_ERROR([Exiting]))
AC_CHECK_HEADERS([signal.h stdlib.h assert.h],[],AC_MSG_ERROR([Exiting]))
AC_CHECK_HEADERS([X11/Xlib.h X11/Intrinsic.h X11/Xutil.h X11/Xos.h X11/xpm.h],[],AC_MSG_ERROR([Exiting]))

PKG_CHECK_MODULES(XML, [libxml-2.0])
PKG_CHECK_MODULES(GTK, [gtk+-3.0 gmodule-2.0])
PKG_CHECK_MODULES(DBUS, [dbus-1])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm gettimeofday sqrt strchr strdup strstr strtol])

AC_CONFIG_FILES([Makefile src/Makefile] src/Pixmaps/Makefile)

AC_OUTPUT
