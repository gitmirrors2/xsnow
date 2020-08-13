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
#include "stars.h"
#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "pixmaps.h"
#include "utils.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())


static int              NStars;  // is copied from Flags.NStars in init_stars. We cannot have that
//                               // NStars is changed outside init_stars
static Pixel            StarcPix[STARANIMATIONS];
static GC               StarGC[STARANIMATIONS];
static Skoordinaten    *Stars = 0;
static char            *StarColor[STARANIMATIONS] = { "gold", "gold1", "gold4", "orange" };
static int              do_stars(void);
static int              do_ustars(void);

static cairo_surface_t *surfaces[STARANIMATIONS];

void stars_init()
{
   int i;
   init_stars();
   if (GtkWinb)
   {
      for(i=0; i<STARANIMATIONS; i++)
      {
	 surfaces[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,9,9);
	 cairo_t *cr = cairo_create(surfaces[i]);
	 cairo_set_line_width(cr,1);
	 //cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
	 GdkRGBA color;
	 gdk_rgba_parse(&color,StarColor[i]);
	 cairo_set_source_rgba(cr,color.red, color.green, color.blue,color.alpha);
	 cairo_move_to(cr, 0, 0 );
	 cairo_line_to(cr, 9, 9 );
	 cairo_move_to(cr, 0, 9 );
	 cairo_line_to(cr, 9, 0 );
	 cairo_move_to(cr, 1, 5 );
	 cairo_line_to(cr, 8, 5 );
	 cairo_move_to(cr, 5, 1 );
	 cairo_line_to(cr, 5, 8 );
	 cairo_stroke(cr);

	 cairo_destroy(cr);
      }
   }
   else
   {
      for (i=0; i<STARANIMATIONS; i++)
      {
	 StarGC[i]  = XCreateGC(display,SnowWin,0,0);
	 StarcPix[i] = IAllocNamedColor(StarColor[i], Black);
      }
      starPix.pixmap = XCreateBitmapFromData(display, SnowWin,
	    (char *)starPix.starBits, starPix.width, starPix.height);
      if (!GtkWinb)
	 add_to_mainloop(PRIORITY_DEFAULT, time_star,           do_stars              ,0);
   }
   add_to_mainloop(PRIORITY_DEFAULT, time_ustar,          do_ustars             ,0);
}

void init_stars()
{
   int i;
   NStars = Flags.NStars;
   P("initstars %d\n",NStars);
   Stars = (Skoordinaten *) realloc(Stars,NStars*sizeof(Skoordinaten));
   //for (i=0; i<Flags.NStars; i++)
   for (i=0; i<NStars; i++)
   {
      Skoordinaten *star = &Stars[i];
      star->x     = randint(SnowWinWidth);
      star->y     = randint(SnowWinHeight/4);
      star->color = randint(STARANIMATIONS);
      P("stars_init %d %d %d\n",star->x,star->y,star->color);
   }
}

void stars_draw(cairo_t *cr)
{
   if (Flags.Done)
      return;
   if (!GtkWinb)
      return;
   int i;
   cairo_save(cr);
   cairo_set_line_width(cr,1);
   //cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
   //cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
   for (i=0; i<NStars; i++)
   {
      P("stars_draw i: %d %d %d\n",i,NStars,counter++);
      Skoordinaten *star = &Stars[i];
      int x = star->x;
      int y = star->y;
      int color = star->color;
      cairo_set_source_surface (cr, surfaces[color], x, y);
      cairo_paint(cr);
   }
   cairo_restore(cr);
}

int stars_ui()
{
   int changes = 0;
   if(Flags.NStars != OldFlags.NStars)
   {
      OldFlags.NStars = Flags.NStars;
      init_stars();
      ClearScreen();
      changes++;
      P("changes NStars: %d %d %d\n",changes,OldFlags.NStars,Flags.NStars);
   }
   return changes;
}


int do_stars()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   int i;
   for (i=0; i<NStars; i++)
   {
      Skoordinaten *star = &Stars[i];
      int x = star->x;
      int y = star->y;
      int k = star->color;
      int w = starPix.width;
      int h = starPix.height;
      P("dostars %d %d %d %d %d %d\n",NStars,x,y,k,w,h);
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
   for (i=0; i<NStars; i++)
      if (drand48() > 0.8)
	 Stars[i].color = randint(STARANIMATIONS);
   return TRUE;
}

void stars_set_gc()
{
   int i;
   for (i=0; i<STARANIMATIONS; i++)
   {
      XSetFunction(   display,StarGC[i],GXcopy);
      XSetStipple(    display,StarGC[i],starPix.pixmap);
      XSetForeground( display,StarGC[i],StarcPix[i]);
      XSetFillStyle(  display,StarGC[i],FillStippled);
   }
}

