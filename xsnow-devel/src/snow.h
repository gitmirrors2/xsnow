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

#define add_flake_to_mainloop(f) add_to_mainloop(PRIORITY_HIGH,time_snowflakes,(GSourceFunc)do_UpdateSnowFlake,f)

#include <gtk/gtk.h>
#include <X11/Intrinsic.h>

#define FLUFFTIME 0.7  // #seconds fluff will live, using GTK/Cairo

typedef struct _Snow {
   int w;                        // width
   int h;                        // height
   float rx;                     // x position
   float ry;                     // y position
   float vx;                     // speed in x-direction, pixels/second
   float vy;                     // speed in y-direction, pixels/second
   float m;                      // mass of flake
   float ivy;                    // initial speed in y direction
   float wsens;                  // wind dependency factor
   float flufftimer;             // fluff timeout timer
   unsigned int whatFlake  : 7;  // snowflake index
   unsigned int cyclic     : 1;  // 1: flake is cyclic 
   unsigned int fluff      : 1;  // 1: flake is in fluff state

} Snow;

typedef struct _SnowMap {
   char *snowBits;
   Pixmap pixmap;
   int width;
   int height;
} SnowMap;

extern Region     NoSnowArea_dynamic;
extern Pixel      SnowcPix;
extern int        MaxSnowFlakeHeight;  /* Biggest flake */
extern int        MaxSnowFlakeWidth;   /* Biggest flake */

extern int        do_initsnow(gpointer data);
extern int        do_UpdateSnowFlake(Snow *flake);
extern Snow      *MakeFlake(void);
extern int        snow_draw(cairo_t *cr);
extern void       snow_init(void);
extern void       snow_set_gc(void);
extern int        snow_ui();
