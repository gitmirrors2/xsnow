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
#pragma once

#include <X11/Intrinsic.h>
#define add_to_mainloop(prio,time,func,datap) g_timeout_add_full(prio,(int)1000*(time),(GSourceFunc)func,datap,0)

extern void    ClearScreen(void);
extern float   fsignf(float x);
extern FILE   *HomeOpen(const char *file,const char *mode,char **path);
extern float   sq2(float x, float y);
extern float   sq3(float x, float y, float z);
extern Pixel  IAllocNamedColor(const char *colorName, Pixel dfltPix);
extern Pixel  AllocNamedColor(const char *colorName, Pixel dfltPix);
extern Pixel Black, White;
