/* -copyright-
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
*/
#pragma once
#include <X11/Xlib.h>
#include "doit.h"

#define DOIT_I(x,d,v) int x; int default_##x; int vintage_##x;
#define DOIT_L(x,d,v) unsigned long int x; unsigned long int default_##x; unsigned long int vintage_##x;
#define DOIT_S(x,d,v) char *x; char *default_##x; char *vintage_##x;

typedef struct _flags {
   DOITALL
      int dummy;
}FLAGS;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S

extern FLAGS Flags;
extern FLAGS OldFlags;

extern int  HandleFlags(int argc, char*argv[]);
extern void InitFlags(void);
extern void WriteFlags(void);
