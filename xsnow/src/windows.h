#pragma once
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
#include <gtk/gtk.h>
#include <X11/Intrinsic.h>
extern Display    *display;
extern int         screen;
extern Window      SnowWin;       // window to snow in
extern Window      SnowWina;      // SnowWin is either SnowWina
extern Window      SnowWinb;      //   or SnowWinb
extern int         SnowWinWidth; 
extern int         SnowWinHeight;
extern int         SnowWinBorderWidth;
extern int         SnowWinDepth;
extern char       *DesktopSession;
extern int         IsCompiz;
extern int         IsWayland;
extern GtkWidget  *drawing_area;
extern GdkWindow  *gdkwindow;
extern Pixel        ErasePixel;
extern int          Exposures;
extern Pixel        BlackPix;
extern GtkWidget   *TransA;  
extern GtkWidget   *TransB;  
extern int          CWorkSpace;  // int? Yes, in compiz we take the placement of the desktop
//                                  which can easily be > 16 bits
extern long         TransWorkSpace;  // workspace on which transparent window is placed
extern char        *SnowWinName;
extern Window       Rootwindow;
extern int          Xroot;
extern int          Yroot;
extern unsigned int Wroot;
extern unsigned int Hroot;
extern int          SnowWinX; 
extern int          SnowWinY; 

extern int          windows_ui(void);
extern void         windows_draw(cairo_t *cr);
extern void         windows_init(void);
extern int          WorkspaceActive(void);  // defined in main.c
extern int          DetermineWindow(Window *xtrans, char **xtransname, GtkWidget **gtrans,const char *transname, int *IsDesktop);
extern void         InitDisplayDimensions(void);
extern void         DestroyWindow(Window w);
extern void         setbelow(GtkWindow *w);
extern void         setabove(GtkWindow *w);
extern void         DisplayDimensions(void);

static const int UW_DEFAULT     = 0; 
static const int UW_TRANSPARENT = 2;

#define ALPHA (0.01*(100 - Flags.Transparency))

extern struct _switches
{
#ifdef NO_USE_BITS
   unsigned int UseGtk    ;
   unsigned int Trans     ;
   unsigned int Root      ;
   unsigned int DrawBirds ;
   unsigned int Exposures ;
   unsigned int Desktop   ;
#else
   unsigned int UseGtk    :1;
   unsigned int Trans     :1;
   unsigned int Root      :1;
   unsigned int DrawBirds :1;
   unsigned int Exposures :1;
   unsigned int Desktop   :1;
#endif
} switches;
