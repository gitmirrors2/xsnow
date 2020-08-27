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

#include <X11/Intrinsic.h>

#define STARANIMATIONS 4

typedef struct _StarMap {
   unsigned char *starBits;
   Pixmap pixmap;
   int width;
   int height;
} StarMap;

typedef struct _Skoordinaten {
   int x; 
   int y; 
   int color; 
} Skoordinaten;

extern void    init_stars(void);
extern void    stars_draw(cairo_t *cr);
extern void    stars_init(void);
extern void    stars_set_gc(void);
extern int     stars_ui(void);
extern void    stars_clear(void);
extern void    stars_reinit(void);

