/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021,2022,2023 Willem Vermin
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

#include <gtk/gtk.h>
#include <X11/Intrinsic.h>

extern void         windows_ui(void);
extern void         windows_draw(void);
extern void         windows_init(void);
extern int          WorkspaceActive(void);
extern void         InitDisplayDimensions();
extern void         setabove(GtkWindow *w);
extern void         DisplayDimensions(void);
extern Window       XWinInfo(char **name);
extern void         UpdateFallenSnowRegions(void);
extern void         UpdateFallenSnowRegionsWithLock(void);
extern void         SetBackground(void);
extern int          xinerama(Display *display, int xscreen, int *x, int *y, int *w, int *h);
