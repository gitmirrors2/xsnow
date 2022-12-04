/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021,2022 Willem Vermin
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
#include "xsnow.h"

extern long int     GetCurrentWorkspace();
extern int          GetWindows(WinInfo **w, int *nw);
extern Window       FindWindowWithName(Display *dsp, const char* needle);
extern WinInfo     *FindWindow(WinInfo *windows, int nwin, Window id);
extern void         printwindows(Display *dpy,WinInfo *windows, int nwin);
extern int          GetProperty32(Display *display, Window window, const char *atomname, 
      const char *needle, int *props, const int nprops);

