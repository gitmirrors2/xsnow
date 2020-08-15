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

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include "debug.h"
#include "flags.h"

cairo_region_t *gSnowOnTreesRegion;
Region SnowOnTreesRegion;

void treesnow_init()
{
}

void treesnow_draw(cairo_t *cr)
{
#define testj
#ifdef testje
   cairo_region_t *region = cairo_region_create();
   cairo_rectangle_int_t rect;
   int i;
   for (i=0; i<5; i++)
   {
      rect.x=1000+100*i;
      rect.y=500+100*i;
      rect.width = 100+10*i;
      rect.height = 20+10*i;
      cairo_region_union_rectangle(region,&rect);
   }
   gdk_cairo_region(cr,region);
   cairo_set_source_rgba(cr,1,0,1,0.5);
   cairo_fill(cr);
   cairo_region_destroy(region);
#endif
   gdk_cairo_region(cr,gSnowOnTreesRegion);
   GdkRGBA color;
   gdk_rgba_parse(&color,Flags.SnowColor);
   cairo_set_source_rgb(cr,color.red,color.green,color.blue);
   cairo_fill(cr);
}

void treesnow_ui()
{
}
