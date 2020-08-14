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

#include <gtk/gtk.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <stdlib.h>
#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "clocks.h"
#include "snow.h"
#include "meteo.h"
#include "utils.h"
#include "xsnow.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

static int do_emeteorite(gpointer data);
static int do_meteorite(gpointer data);

static GdkRGBA       color;
static const char   *MeteoColor  = "orange";
static Pixel         MeteoPix;
static MeteoMap      meteorite;

void meteo_init()
{
   if (GtkWinb)
   {
      if (!gdk_rgba_parse(&color, MeteoColor))
	 gdk_rgba_parse(&color,"rgb(255,165,0)");
   }
   else
   {
      meteorite.gc  = XCreateGC(display, SnowWin, 0, 0);
      meteorite.egc = XCreateGC(display, SnowWin, 0, 0);
      MeteoPix = IAllocNamedColor(MeteoColor, White);
      XSetLineAttributes(display, meteorite.gc,  1,LineSolid,CapRound,JoinMiter);
      XSetLineAttributes(display, meteorite.egc, 1,LineSolid,CapRound,JoinMiter);
      if(UseAlpha)
      {
	 XSetFunction(display,   meteorite.gc,  GXcopy);
	 XSetForeground(display, meteorite.gc,  MeteoPix);
	 XSetFunction(display,   meteorite.egc, GXcopy);
	 XSetForeground(display, meteorite.egc, ErasePixel);
      }
      else
      {
	 XSetFunction(display,   meteorite.gc,  GXxor);
	 XSetForeground(display, meteorite.gc,  MeteoPix);
	 XSetFunction(display,   meteorite.egc, GXxor);
	 XSetForeground(display, meteorite.egc, MeteoPix);
      }
   }
   add_to_mainloop(PRIORITY_DEFAULT, time_emeteorite,     do_emeteorite         ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_meteorite,      do_meteorite          ,0);
}

int meteo_ui()
{
   int changes = 0;
   if(Flags.NoMeteorites != OldFlags.NoMeteorites)
   {
      OldFlags.NoMeteorites = Flags.NoMeteorites;
      changes++;
      P("changes: %d %d\n",changes,Flags.NoMeteorites);
   }
   return changes;
}

void meteo_draw(cairo_t *cr)
{
   if(!GtkWinb)
      return;
   if (!meteorite.active)
      return;
   cairo_save(cr);

   cairo_set_source_rgb(cr,color.red, color.green, color.blue);
   cairo_set_line_width(cr,2);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
   cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
   cairo_move_to(cr,meteorite.x1,meteorite.y1);
   cairo_line_to(cr,meteorite.x2,meteorite.y2);
   cairo_stroke(cr);

   cairo_restore(cr);
}

int do_emeteorite(gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (!meteorite.active || NOTACTIVE || Flags.NoMeteorites)
      return TRUE;
   if (wallclock() - meteorite.starttime > 0.3)
   {
      if (!GtkWinb)
      {
	 XDrawLine(display, SnowWin, meteorite.egc,  
	       meteorite.x1,meteorite.y1,meteorite.x2,meteorite.y2);
	 XSubtractRegion(NoSnowArea_dynamic ,meteorite.r,NoSnowArea_dynamic);
	 XDestroyRegion(meteorite.r);
	 XFlush(display);
      }
      meteorite.active = 0;
   }
   return TRUE;
}

int do_meteorite(gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (meteorite.active) return TRUE;
   if(Flags.NoMeteorites) return TRUE;
   if (drand48() > 0.2) return TRUE;

   meteorite.x1 = randint(SnowWinWidth);
   meteorite.y1 = randint(SnowWinHeight/4);
   meteorite.x2 = meteorite.x1 + SnowWinWidth/10 - randint(SnowWinWidth/5);
   if (meteorite.x2 == meteorite.x1)
      meteorite.x2 +=5;
   meteorite.y2 = meteorite.y1 + SnowWinHeight/5 - randint(SnowWinHeight/5);
   if (meteorite.y2 == meteorite.y1)
      meteorite.y2 +=5;
   meteorite.active  = 1;
   if(!GtkWinb)
   {
      const int npoints = 5;
      XPoint points[npoints];

      points[0].x = meteorite.x1+1;
      points[0].y = meteorite.y1-1;
      points[1].x = meteorite.x2+1;
      points[1].y = meteorite.y2-1;
      points[2].x = meteorite.x2-1;
      points[2].y = meteorite.y2+1;
      points[3].x = meteorite.x1-1;
      points[3].y = meteorite.y1+1;
      points[4].x = meteorite.x1+1;
      points[4].y = meteorite.y1-1;

      meteorite.r = XPolygonRegion(points,npoints,EvenOddRule);
      XUnionRegion(meteorite.r,NoSnowArea_dynamic,NoSnowArea_dynamic);
      XDrawLine(display, SnowWin, meteorite.gc, 
	    meteorite.x1,meteorite.y1,meteorite.x2,meteorite.y2);
      XFlush(display);
   }

   meteorite.starttime = wallclock();
   return TRUE;
}
