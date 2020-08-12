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
#include <X11/Intrinsic.h>
#include "stars.h"
#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "pixmaps.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

Skoordinaten *Stars = 0;
GC StarGC[STARANIMATIONS];
char *StarColor[STARANIMATIONS] = { "gold", "gold1", "gold4", "orange" };

void stars_init()
{
   initstars();
   if (!GtkWinb)
      add_to_mainloop(PRIORITY_DEFAULT, time_star,           do_stars              ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_ustar,          do_ustars             ,0);
}

void initstars()
{
   int i;
   R("initstars %d\n",Flags.NStars);
   Stars = (Skoordinaten *) realloc(Stars,Flags.NStars*sizeof(Skoordinaten));
   for (i=0; i<Flags.NStars; i++)
   {
      Skoordinaten *star = &Stars[i];
      star->x     = drand48()*SnowWinWidth;
      star->y     = drand48()*(SnowWinHeight/4);
      star->color = drand48()*STARANIMATIONS;
      gdk_rgba_parse(&Stars[i].gcolor,StarColor[star->color]);
      P("stars_init %d %d %d\n",star->x,star->y,star->color);
   }
}

void stars_draw(cairo_t *cr)
{
   if (!GtkWinb)
      return;
   int i;
   static int counter = 0;
   cairo_save(cr);
   cairo_set_line_width(cr,1);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
   cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
   for (i=0; i<Flags.NStars; i++)
   {
      R("stars_draw i: %d %d %d\n",i,Flags.NStars,counter++);
      Skoordinaten *star = &Stars[i];
      int x = star->x;
      int y = star->y;
      GdkRGBA color = star->gcolor;
      cairo_set_source_rgb(cr,color.red, color.green, color.blue);
      cairo_move_to(cr,x  ,y);
      cairo_line_to(cr,x+8,y+8);
      cairo_move_to(cr,x  ,y+8);
      cairo_line_to(cr,x+8,y  );
      cairo_move_to(cr,x+1,y+4);
      cairo_line_to(cr,x+7,y+4);
      cairo_move_to(cr,x+4,y+1);
      cairo_line_to(cr,x+4,y+7);
      cairo_stroke(cr);
   }
   cairo_restore(cr);
}

int stars_ui()
{
   int changes = 0;
   return changes;
}


int do_stars()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   int i;
   for (i=0; i<Flags.NStars; i++)
   {
      Skoordinaten *star = &Stars[i];
      int x = star->x;
      int y = star->y;
      int k = star->color;
      int w = starPix.width;
      int h = starPix.height;
      P("dostars %d %d %d %d %d %d\n",Flags.NStars,x,y,k,w,h);
      XSetTSOrigin(display, StarGC[k],x+w, y+h);
      XFillRectangle(display,SnowWin,StarGC[k],x,y,w,h);
   }
   XFlush(display);
   return TRUE;
}

int do_ustars()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   int i;
   for (i=0; i<Flags.NStars; i++)
   {
      if (drand48() > 0.8)
      {
	 int k = drand48()*STARANIMATIONS;
	 Stars[i].color = k;
	 gdk_rgba_parse(&Stars[i].gcolor,StarColor[k]);
      }
   }
   return TRUE;
}
