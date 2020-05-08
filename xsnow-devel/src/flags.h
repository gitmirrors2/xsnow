/* -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019,2020 Willem Vermin
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
   //unsigned long int WindowId;
   DOITALL
      int dummy;
}FLAGS;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S

#define FVWMFLAGS \
   do { \
      Flags.UseAlpha  = 0; \
      Flags.UseBG     = 0; \
      Flags.Exposures = 0; } while(0)

#define GNOMEFLAGS \
   do { \
      Flags.UseAlpha  = 1; \
      Flags.UseBG     = 0; \
      Flags.Exposures = 0; } while(0)


extern FLAGS Flags;

extern int  HandleFlags(int argc, char*argv[]);
extern void InitFlags(void);
extern void PrintVersion(void);
extern void WriteFlags(void);
#endif
