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
static int do_meteorite(void *);

static GdkRGBA       color;
static const char   *MeteoColor  = "orange";
static MeteoMap      meteorite;

void meteo_init()
{
   if (!gdk_rgba_parse(&color, MeteoColor))
      gdk_rgba_parse(&color,"rgb(255,165,0)");
   add_to_mainloop1(PRIORITY_DEFAULT, time_emeteorite, do_emeteorite, NULL);
   add_to_mainloop (PRIORITY_DEFAULT, time_meteorite,  do_meteorite);
}

void meteo_ui()
{
   UIDO(NoMeteorites   , );
}

void meteo_draw(cairo_t *cr)
{
   P("meteo_draw %d %d\n",counter++,meteorite.active);
   if (!meteorite.active)
      return;
   cairo_save(cr);

   cairo_set_source_rgba(cr,color.red, color.green, color.blue, ALPHA);
   cairo_set_line_width(cr,2);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
   cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
   cairo_move_to(cr,meteorite.x1,meteorite.y1);
   cairo_line_to(cr,meteorite.x2,meteorite.y2);
   cairo_stroke(cr);

   cairo_restore(cr);
}

void meteo_erase()
{
   do_emeteorite(NULL);
}

int do_emeteorite(gpointer data)
{
   (void)data;
   if (Flags.Done)
      return FALSE;
   if (!meteorite.active || NOTACTIVE || Flags.NoMeteorites)
      return TRUE;
   if (wallclock() - meteorite.starttime > 0.3)
   {
      if (!global.IsDouble)
      {
	 int x = meteorite.x1;
	 int y = meteorite.y1;
	 int w = meteorite.x2 - x;
	 int h = meteorite.y2 - y;
	 if (w<0)
	 {
	    x += w;
	    w = -w;
	 }
	 if (h<0)
	 {
	    y += h;
	    h = -h;
	 }
	 x -= 1;
	 y -= 1;
	 w += 2;
	 h += 2;
	 myXClearArea(global.display,global.SnowWin,x,y,w,h,global.xxposures);
      }
      meteorite.active = 0;
   }
   return TRUE;
}

int do_meteorite(void *d)
{
   (void)d;
   P("do_meteorite %d\n",counter++);
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (meteorite.active) return TRUE;
   if(Flags.NoMeteorites) return TRUE;
   if (drand48() > 0.2) return TRUE;

   meteorite.x1 = randint(global.SnowWinWidth);
   meteorite.y1 = randint(global.SnowWinHeight/4);
   meteorite.x2 = meteorite.x1 + global.SnowWinWidth/10 - randint(global.SnowWinWidth/5);
   if (meteorite.x2 == meteorite.x1)
      meteorite.x2 +=5;
   meteorite.y2 = meteorite.y1 + global.SnowWinHeight/5 - randint(global.SnowWinHeight/5);
   if (meteorite.y2 == meteorite.y1)
      meteorite.y2 +=5;
   meteorite.active  = 1;

   meteorite.starttime = wallclock();
   return TRUE;
}
