/* 
   -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024 Willem Vermin
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
   (char *)"snow2", (char *)"snow3" };
static int              do_ustars(void *);
static void             set_star_surfaces(void);

#ifdef CIRCLESTARS  // suggestion by Mihai Dobrescu
static const double   StarSize = 1.5;
//static const double   StarSize = 30;     // stardebug
#else
static const int   StarSize = 9;
#endif
static const float LocalScale = 0.8;

static cairo_surface_t *surfaces[STARANIMATIONS];

void stars_init()
{
   init_stars();
   for (int i=0; i<STARANIMATIONS; i++)
      surfaces[i] = NULL;
   set_star_surfaces();
   add_to_mainloop(PRIORITY_DEFAULT, time_ustar, do_ustars);
}

void set_star_surfaces()
{
   for (int i=0; i<STARANIMATIONS; i++)
   {
      float size = LocalScale*global.WindowScale*0.01*Flags.Scale*StarSize;
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
      double haloR = r*4;
      double midpoint = haloR;
      surfaces[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,2*haloR,2*haloR);
#else
      surfaces[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,size,size);
#endif
      cairo_t *cr = cairo_create(surfaces[i]);
      GdkRGBA color;
      gdk_rgba_parse(&color,StarColor[i]);
      cairo_set_source_rgba(cr,color.red, color.green, color.blue,color.alpha);
#ifdef CIRCLESTARS
      {
	 //cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
	 cairo_arc(cr,midpoint,midpoint,r,0,2*M_PI);
	 cairo_close_path(cr);
	 cairo_fill(cr);

	 // create halo
	 cairo_pattern_t *pattern;
	 pattern = cairo_pattern_create_radial(midpoint, midpoint, r, midpoint, midpoint, haloR);  
	 //double bright = 25 * ALPHA * 0.01;
	 //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 234.0/255, 244.0/255, 252.0/255, bright);
	 //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 234.0/255, 244.0/255, 252.0/255, 0.0);
	 cairo_pattern_add_color_stop_rgba(pattern, 0.0, color.red, color.green, color.blue, 1.0);
	 cairo_pattern_add_color_stop_rgba(pattern, 1.0, color.red, color.green, color.blue, 0.0);
	 //cairo_surface_t *halo_surface;
	 //halo_surface      = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2*haloR, 2*haloR);
	 cairo_t *halocr = cr;
	 cairo_set_source_rgba(halocr, 0, 0, 0, 0);
	 cairo_paint          (halocr);
	 cairo_set_source     (halocr, pattern);
	 cairo_arc            (halocr, midpoint, midpoint, haloR, 0, M_PI * 2);
	 cairo_fill           (halocr);     
	 cairo_pattern_destroy  (pattern);

      }
#else
      {
	 cairo_set_line_width(cr,1.0*size/StarSize);
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
      myXClearArea(global.display,global.SnowWin,x,y,StarSize,StarSize,global.xxposures);
   }
}

void stars_ui()
{
   UIDO(NStars, init_stars(); ClearScreen(););
   UIDO(Stars, ClearScreen(););

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
      if (drand48() > 0.8)
	 Stars[i].color = randint(STARANIMATIONS);
   return TRUE;
   (void)d;
}

