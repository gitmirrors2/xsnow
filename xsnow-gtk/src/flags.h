/* -copyright-
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
*/
#ifndef FLAGS_H
#define FLAGS_H
#include <X11/Xlib.h>
#include "doit.h"

#define DOIT_I(x) int x; 
#define DOIT_L(x) unsigned long int x;
#define DOIT_S(x) char *x;

typedef struct flags {
   DOITALL
}FLAGS;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S

#define FVWMFLAGS \
   do { \
      flags.usealpha  = 0; \
      flags.usebg     = 0; \
      flags.exposures = 0; } while(0)

#define GNOMEFLAGS \
   do { \
      flags.usealpha  = 1; \
      flags.usebg     = 0; \
      flags.exposures = 0; } while(0)

#define KDEFLAGS \
   do { \
      flags.usealpha  = 1; \
      flags.usebg     = 0; \
      flags.exposures = 0; } while(0)

extern FLAGS flags;
extern int   HandleFlags(int argc, char*argv[]);
extern void  InitFlags(void);
extern void  PrintVersion(void);
extern void  WriteFlags(void);
#endif
