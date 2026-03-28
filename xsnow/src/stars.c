/* 
   -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024,2025,2026 Willem Vermin
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#-endcopyright-
*/
#include <pthread.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>

#include "xsnow-constants.h"

#include "stars.h"
#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "pixmaps.h"
#include "utils.h"
#include "safe_malloc.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

#define CIRCLESTARS


static int              NStars;  // is copied from Flags.NStars in init_stars. We cannot have that
				 //                               // NStars is changed outside init_stars
static Skoordinaten    *Stars = NULL;
static char            *StarColor[STARANIMATIONS] = { (char *)"white", (char *)"snow", 
   (char *)"snow2", (char *)"yellow" };
static int              do_ustars(void *);
static void             set_star_surfaces(void);

#ifdef CIRCLESTARS  // suggestion by Mihai Dobrescu
static const double   STARSIZE = 3;
//static const double   STARSIZE = 30;     // stardebug
#else
static const int   STARSIZE = 9;
#endif
static const float LocalScale = 0.8;

static cairo_surface_t *surfaces[STARANIMATIONS];

void stars_init()
{
   init_stars();
   for (int i=0; i<STARANIMATIONS; i++)
      surfaces[i] = NULL;
   set_star_surfaces();
   //add_to_mainloop(PRIORITY_DEFAULT, time_ustar, do_ustars);
   add_to_mainloop(PRIORITY_DEFAULT, time_umoon, do_ustars);
}

void set_star_surfaces()
{
   for (int i=0; i<STARANIMATIONS; i++)
   {
      float size = LocalScale*global.WindowScale*0.01*Flags.Scale*STARSIZE*Flags.StarSize*0.015;
#ifdef CIRCLESTARS
      size *= 0.25*(1+4*drand48());
#else
      size *= 0.2*(1+4*drand48());
#endif
      if (size < 1 ) size = 1;
      if(surfaces[i])
	 cairo_surface_destroy(surfaces[i]);
#ifdef CIRCLESTARS
      double r = size/2;
      double midpoint = r;
      //surfaces[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,2*haloR,2*haloR);
      P("size: %f r:%f\n",size,r);
      surfaces[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,2*r,2*r);
#else
      surfaces[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,size,size);
#endif
      cairo_t *cr = cairo_create(surfaces[i]);
      GdkRGBA color;
      gdk_rgba_parse(&color,StarColor[i]);
      cairo_set_source_rgba(cr,color.red, color.green, color.blue,color.alpha);
#ifdef CIRCLESTARS
      {
	 cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
	 cairo_arc(cr,midpoint,midpoint,r,0,2*M_PI);
	 cairo_close_path(cr);
	 cairo_fill(cr);
      }
#else
      {
	 cairo_set_line_width(cr,1.0*size/STARSIZE);
	 cairo_move_to(cr, 0           , 0 );
	 cairo_line_to(cr, size        , size );
	 cairo_move_to(cr, 0           , size );
	 cairo_line_to(cr, size        , 0 );
	 cairo_move_to(cr, 0           , size/2 );
	 cairo_line_to(cr, size        , size/2 );
	 cairo_move_to(cr, size/2      , 0 );
	 cairo_line_to(cr, size/2      , size );
	 cairo_stroke(cr);
      }
#endif

      cairo_destroy(cr);
   }
}


void init_stars()
{
   NStars = Flags.NStars;
   P("initstars %d\n",NStars);
   // Nstars+1: we do not allocate 0 bytes
   Stars = (Skoordinaten *) realloc(Stars,(NStars+1)*sizeof(Skoordinaten));
   REALLOC_CHECK(Stars);
   for (int i=0; i<NStars; i++)
   {
      Skoordinaten *star = &Stars[i];
      star->x     = randint(global.SnowWinWidth);
      star->y     = randint(global.SnowWinHeight/4);
      star->color = randint(STARANIMATIONS);
      P("stars_init %d %d %d\n",star->x,star->y,star->color);
   }
   //set_star_surfaces();
}

void stars_draw(cairo_t *cr)
{
   if (!Flags.Stars)
      return;
   cairo_save(cr);
   cairo_set_line_width(cr,1);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
   for (int i=0; i<NStars; i++)
   {
      P("stars_draw i: %d %d %d\n",i,NStars,counter++);
      Skoordinaten *star = &Stars[i];
      int x = star->x;
      int y = star->y;
      int color = star->color;
      cairo_set_source_surface (cr, surfaces[color], x, y);
      my_cairo_paint_with_alpha(cr,ALPHA);
   }

   cairo_restore(cr);
}

void stars_erase()
{
   if (!Flags.Stars)
      return;
   for (int i=0; i<NStars; i++)
   {
      P("stars_erase i: %d %d %d\n",i,NStars,counter++);
      Skoordinaten *star = &Stars[i];
      int x = star->x;
      int y = star->y;
      myXClearArea(global.display,global.SnowWin,x,y,STARSIZE,STARSIZE,global.xxposures);
   }
}

void stars_ui()
{
   UIDO(NStars, init_stars(); ClearScreen(););
   UIDO(Stars, ClearScreen(););
   UIDO(StarSize, set_star_surfaces(); init_stars(); ClearScreen(););

   static int prev = 100;
   P("stars_ui %d\n",prev);
   if(ScaleChanged(&prev))
   {
      set_star_surfaces();
      init_stars();
      P("stars_ui changed\n");
   }
}

int do_ustars(void *d)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   for (int i=0; i<NStars; i++)
   {
      Stars[i].x += 0.8*time_umoon*Flags.MoonSpeed/60.0;
      P("dx: %f\n",time_umoon*Flags.MoonSpeed/60.0);

      if (Stars[i].x > global.SnowWinWidth)
	 Stars[i].x = 0;

      if (drand48() > 0.95)
	 Stars[i].color = randint(STARANIMATIONS);
      // Note: color not only defines color but also size
   }
   return TRUE;
   (void)d;
}

