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

#define add_flake_to_mainloop(f) add_to_mainloop1(PRIORITY_HIGH,time_snowflakes,(GSourceFunc)do_UpdateSnowFlake,f)

#include <gtk/gtk.h>
#include <X11/Intrinsic.h>


typedef struct _Snow {
   float rx;                     // x position
   float ry;                     // y position
   float vx;                     // speed in x-direction, pixels/second
   float vy;                     // speed in y-direction, pixels/second
   float m;                      // mass of flake
   float ivy;                    // initial speed in y direction
   float wsens;                  // wind dependency factor
   float flufftimer;             // fluff timeout timer
   float flufftime;              // fluff timeout
   unsigned int whatFlake;       // snowflake index
#ifdef NO_USE_BITS 
   unsigned int cyclic     ;  // flake is cyclic 
   unsigned int fluff      ;  // flake is in fluff state
   unsigned int freeze     ;  // flake does not move
   unsigned int testing    ;  // for testing purposes
#else
   unsigned int cyclic     : 1;  // flake is cyclic 
   unsigned int fluff      : 1;  // flake is in fluff state
   unsigned int freeze     : 1;  // flake does not move
   unsigned int testing    : 2;  // for testing purposes
#endif

} Snow;

typedef struct _SnowMap {
   Pixmap pixmap;
#ifdef NO_USE_BITS 
   unsigned int width       ;
   unsigned int height      ;
#else
   unsigned int width   : 16;
   unsigned int height  : 16;
#endif
} SnowMap;

extern Region       NoSnowArea_dynamic;
extern Pixel        SnowcPix;
extern unsigned int MaxSnowFlakeHeight;  /* Biggest flake */
extern unsigned int MaxSnowFlakeWidth;   /* Biggest flake */
extern int          FlakeCount;          /* number of flakes */
extern int          FluffCount;          /* number of fluff flakes */

extern int        do_initsnow(void);
extern int        do_UpdateSnowFlake(Snow *flake);
extern Snow      *MakeFlake(int type);
extern int        snow_draw(cairo_t *cr);
extern void       snow_init(void);
extern void       snow_set_gc(void);
extern void       snow_ui();
extern void       fluffify(Snow *flake, float t);
extern void       printflake(Snow *flake);
