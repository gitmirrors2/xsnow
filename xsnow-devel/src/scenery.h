#pragma once
/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020 Willem Vermin
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

#include "xsnow.h"
#include <gtk/gtk.h>

typedef struct Treeinfo { 
   int x;                    // x position
   int y;                    // y position
   unsigned int type:8;      // type (TreeType, -treetype)
   unsigned int rev :1;      // reversed
} Treeinfo;

extern void scenery_init(void);
extern int      scenery_ui(void);
extern void     EraseTrees(void);
extern void     scenery_set_gc(void);
extern int      scenery_draw(cairo_t *cr);

extern int      KillTrees;  // 1: signal to trees to kill themselves
extern Region   TreeRegion;

