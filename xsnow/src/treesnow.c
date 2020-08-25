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
#include "utils.h"
#include "windows.h"
#include "xsnow.h"
#include "scenery.h"
#include "wind.h"
#include "snow.h"
#include "blowoff.h"
#include "treesnow.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

// we need both type of regions because the region is painted to
cairo_region_t *gSnowOnTreesRegion;
Region          SnowOnTreesRegion;

static          GC SnowOnTreesGC;
XPoint         *SnowOnTrees = 0;
int            OnTrees = 0;

static int do_snow_on_trees(gpointer data);
static void   ConvertOnTreeToFlakes(void);

void treesnow_init()
{
   SnowOnTreesGC        = XCreateGC(display, SnowWin,    0, 0);
   SnowOnTreesRegion    = XCreateRegion();
   gSnowOnTreesRegion   = cairo_region_create();
   add_to_mainloop(PRIORITY_DEFAULT, time_snow_on_trees,  do_snow_on_trees      ,0);
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
   if (Flags.NoKeepSnowOnTrees || Flags.NoTrees)
      return;
   GdkRGBA color;
   gdk_rgba_parse(&color,Flags.SnowColor);
   cairo_set_source_rgb(cr,color.red,color.green,color.blue);
   gdk_cairo_region(cr,gSnowOnTreesRegion);
   cairo_fill(cr);
}

int treesnow_ui()
{
   int changes = 0;
   if(Flags.MaxOnTrees != OldFlags.MaxOnTrees)
   {
      OldFlags.MaxOnTrees = Flags.MaxOnTrees;
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoKeepSnowOnTrees != OldFlags.NoKeepSnowOnTrees)
   {
      OldFlags.NoKeepSnowOnTrees = Flags.NoKeepSnowOnTrees;
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   return changes;
}

int do_snow_on_trees(gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE || KillTrees)
      return TRUE;
   if(Flags.NoKeepSnowOnTrees || Flags.NoTrees)
      return TRUE;
   if (Wind == 2)
      ConvertOnTreeToFlakes();
   static int second = 0;

   if (switches.UseGtk)
   {
   // for gtk, drawing is done in treesnow_draw()
   }
   else
   {
      if (second)
      {
	 second = 1;
	 XSetForeground(display, SnowOnTreesGC, ~BlackPix); 
	 XFillRectangle(display, SnowWin, SnowOnTreesGC, 0,0,SnowWinWidth,SnowWinHeight);
      }
      XSetRegion(display, SnowOnTreesGC, SnowOnTreesRegion);
      XSetForeground(display, SnowOnTreesGC, SnowcPix); 
      XFillRectangle(display, SnowWin, SnowOnTreesGC, 0,0,SnowWinWidth,SnowWinHeight);
   }
   return TRUE;
}


void  treesnow_set_gc()
{
   XSetFunction(display, SnowOnTreesGC, GXcopy);
}

// blow snow off trees
void ConvertOnTreeToFlakes()
{
   if(Flags.NoKeepSnowOnTrees || Flags.NoBlowSnow || Flags.NoTrees)
      return;
   int i;
   for (i=0; i<OnTrees; i++)
   {
      int j;
      for (j=0; j<3; j++)
      {
	 int k, kmax = BlowOff();
	 for (k=0; k<kmax; k++)
	 {
	    Snow *flake   = MakeFlake();
	    flake->rx     = SnowOnTrees[i].x;
	    flake->ry     = SnowOnTrees[i].y-5*j;
	    flake->vy     = -10;
	    flake->cyclic = 0;
	    add_flake_to_mainloop(flake);
	 }
      }
   }
   OnTrees = 0;
   reinit_treesnow_region();
}

void reinit_treesnow_region()
{
   XDestroyRegion(SnowOnTreesRegion);
   SnowOnTreesRegion = XCreateRegion();
   cairo_region_destroy(gSnowOnTreesRegion);
   gSnowOnTreesRegion = cairo_region_create();
}

void InitSnowOnTrees()
{
   SnowOnTrees = (XPoint *)realloc(SnowOnTrees,sizeof(*SnowOnTrees)*Flags.MaxOnTrees);
   if (OnTrees > Flags.MaxOnTrees)
      OnTrees = Flags.MaxOnTrees;
}