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
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <gtk/gtk.h>
#include <stdlib.h>

typedef struct FallenSnow {
   Window id;                       // window id, 0 for snow at bottom
   int                x,y;          // Coordinates of fallen snow, y for bottom
   int                w,h;          // width, max height of fallen snow
   int                w8;           // width rounded up to 8-fold
   short int         *acth;         // actual height
   short int         *desh;         // desired height
   short int          ws;           // visible on workspace ws
   struct FallenSnow *next;         // pointer to next item
   cairo_surface_t   *surface;      // 
   unsigned int       hidden : 1;   // if True, the window is hidden (iconized)
   unsigned int       clean  : 1;   // if True, this area has been cleaned
   unsigned int       sticky : 1;   // visible on all workspaces
} FallenSnow;

extern FallenSnow *FsnowFirst;

extern void   UpdateFallenSnowPartial(FallenSnow *fsnow, int x, int w); // used in snow.c
extern int    HandleFallenSnow(FallenSnow *fsnow);


extern void   fallensnow_init(void);
extern void   fallensnow_draw(cairo_t *cr);
extern int    fallensnow_ui(void);
extern void   CleanFallenArea(FallenSnow *fsnow, int x, int w);
extern void   CleanFallen(Window id);
extern void   DrawFallen(FallenSnow *fsnow);
extern void   GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy);
extern void   InitFallenSnow(void);
extern void   UpdateFallenSnowWithWind(FallenSnow *fsnow,int w, int h);
extern int    do_fallen(gpointer data);
extern void   SetMaxScreenSnowDepth(void);
extern void   fallensnow_set_gc(void);


// insert a node at the start of the list
extern void PushFallenSnow(FallenSnow **first, int window_id, int ws, int sticky,
      int x, int y, int w, int h);

// pop first element
extern int PopFallenSnow(FallenSnow **list);

// remove by id
extern int RemoveFallenSnow(FallenSnow **list, Window id);

// print list
extern void PrintFallenSnow(FallenSnow *list);

// free fallen
extern void FreeFallenSnow(FallenSnow *fallen);

// find fallensnow with id
extern FallenSnow *FindFallen(FallenSnow *first, Window id);

extern int        MaxScrSnowDepth;
