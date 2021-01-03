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
#include "doitb.h"
extern struct _globals {
   int   maxix, maxiy, maxiz;   // [pixels]
   float maxx,  maxy,  maxz;    // [m]
   float ax, ay, az;            // [pixels/m]   see r2i in main.c for usage
   float ox, oy, oz;            // [m]           idem
   float maxrange;              // max distanc to look for other birds [m]
   float bird_scale;            // scale for drawing birds
   float prefdweight;           // dimensionless
   float meanspeed;             // [m/s]      preferred mean speed of birds
   int neighbours_max;          // max number of neighbours to look at
   float range;                 // range wherein neighbours are to be found [m]
   float mean_distance;         // mean distance [m]
   //float vd;                    // viewing distance (camera obscura) [m]
   float xc, zc;                // coordinates of camera obscura lens
#ifdef NO_USE_BITS 
   unsigned int freeze   ;      // when true, system freezes
#else
   unsigned int freeze :1;      // when true, system freezes
#endif

#define DOITB(what,type) \
   type what; \
   type what ## _new; 

   DOITALLB()
#undef DOITB
#ifdef NO_USE_BITS 
#define DOITB(what,type) \
      unsigned int what ## _changed    ;
#else
#define DOITB(what,type) \
      unsigned int what ## _changed  : 1;
#endif

      DOITALLB()
#undef DOITB

#ifdef NO_USE_BITS 
#define DOITB(what) \
      unsigned int what ## _requested    ;
#else
#define DOITB(what) \
      unsigned int what ## _requested  : 1;
#endif
      BUTTONALL()
#undef DOITB

} globals;

