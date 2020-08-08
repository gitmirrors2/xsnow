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
#include <gtk/gtk.h>
#include <X11/Intrinsic.h>
extern Display *display;
extern int screen;
extern Window SnowWin;
extern int SnowWinWidth; 
extern int SnowWinHeight;
extern int SnowWinBorderWidth;
extern int SnowWinDepth;
extern char *DesktopSession;
extern int IsCompiz;
extern int IsWayland;
extern int WorkspaceActive(void);  // defined in main.c
extern GtkWidget       *drawing_area;
extern GdkWindow       *gdkwindow;
extern int       UseAlpha;
extern Pixel   ErasePixel;
extern int          Exposures;
extern Pixel   BlackPix;
extern GtkWidget *GtkWinb;  // for birds
