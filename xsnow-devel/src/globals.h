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
#ifndef GLOBALS_H
#define GLOBALS_H
#include "doitb.h"
extern struct _globals {
   int   maxix, maxiy, maxiz;   // [pixels]
   float maxx,  maxy,  maxz;    // [m]
   float ax, ay, az;            // [pixels/m]   see r2i in main.c for usage
   float ox, oy, oz;            // [m]           idem
   float maxrange;              // max distanc to look for other birds [m]
   float bird_scale;            // scale for drawing birds
   float attrx, attry, attrz;   // [m]          attraction point
   float prefdweight;           // dimensionless
   float meanspeed;             // [m/s]      preferred mean speed of birds
   float meanspeed_new;
   int neighbours_max;          // max number of neighbours to look at
   float range;                 // range wherein neighbours are to be found [m]
   float mean_distance;         // mean distance [m]
   //float vd;                    // viewing distance (camera obscura) [m]
   float xc, zc;                // coordinates of camera obscura lens
   unsigned int freeze :1;      // when true, system freezes

#define DOITB(what,type) \
   type what; \
   type what ## _new; 

   DOITALLB()
#undef DOITB
#define DOITB(what,type) \
      unsigned int what ## _changed  :1;

      DOITALLB()
#undef DOITB

#define DOITB(what) \
      unsigned int what ## _requested  :1;

      BUTTONALL()
#undef DOITB

} globals;

#endif  /* GLOBALS_H */
