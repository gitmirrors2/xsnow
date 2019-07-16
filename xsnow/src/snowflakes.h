/* -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019 Willem Vermin
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
#ifndef SNOWFLAKES_H
#define SNOWFLAKES_H

#include <X11/Xlib.h>
typedef struct Snow {
   int w;                   // width
   int h;                   // height
   float rx;                // x position
   float ry;                // y position
   float vx;                // speed in x-direction, pixels/second
   float vy;                // speed in y-direction, pixels/second
   float m;                 // mass of flake
   float ivy;               // initial speed in y direction
   float wsens;             // wind dependency factor
   unsigned int cyclic : 1; // 0: flake is not cyclic 
   int whatFlake;           // snowflake index
   struct Snow *prev;       // pointer to previous snow flake
   struct Snow *next;       // pointer to next snow flake 
} Snow;

// Add snow flake before flake.
// afterwards, *flake will point to the new flake,
// return value is also this *flake
extern Snow *addFlake(Snow *flake);

// Create a snow flake, prev and next pointers set to 0
extern Snow *createFlake();

// delete flake, return value is flake->next
extern Snow *delFlake(Snow *flake);

// prints flakes, starting with flake
extern void printSnow(Snow *flake);
#endif

