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
#include <math.h>
#include "debug.h"
#include "flags.h"
#include "moon.h"
#include "pixmaps.h"
#include "utils.h"
#include "windows.h"


#define LEAVE_IF_INACTIVE\
   if (!Flags.Moon || !WorkspaceActive()) return TRUE

static int  do_umoon(void);
static void init_moon_surface(void);
static void init_halo_surface(void);
static void halo_draw(cairo_t *cr);

static cairo_surface_t *moon_surface = NULL;
static cairo_surface_t *halo_surface = NULL;
static double moonR;
static double haloR;

double moonX = 1000;
double moonY = 80;

int NMOONPIXBUFS;

void moon_init(void)
{
   init_moon_surface();
   add_to_mainloop(PRIORITY_DEFAULT, time_umoon, do_umoon);
   if (SnowWinWidth > 400)
      moonX = 200+drand48()*(SnowWinWidth - 400 - Flags.MoonSize);
}

int moon_draw(cairo_t *cr)
{
   LEAVE_IF_INACTIVE;
   P("moon_draw %d\n",counter++);
   cairo_set_source_surface (cr, moon_surface, moonX, moonY);
   my_cairo_paint_with_alpha(cr,ALPHA);
   halo_draw(cr);
   return TRUE;
}


void moon_ui()
{
   UIDO(MoonSpeed,                         );
   UIDO(Halo,                              );
   UIDO(Moon,                              );
   UIDO(MoonSize    ,init_moon_surface();  );
   UIDO(HaloBright  ,init_halo_surface();  );
}

static void init_moon_surface()
{
   const GdkInterpType interpolation = GDK_INTERP_HYPER; 
   int whichmoon = 0;
   static GdkPixbuf *pixbuf, *pixbufscaled;
   pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)moons_xpm[whichmoon]);
   int w = Flags.MoonSize;
   int h = Flags.MoonSize;
   if(moon_surface)
      cairo_surface_destroy(moon_surface);
   pixbufscaled = gdk_pixbuf_scale_simple(pixbuf,w,h,interpolation);
   moon_surface = gdk_cairo_surface_create_from_pixbuf (pixbufscaled, 0, NULL);
   g_clear_object(&pixbuf);
   g_clear_object(&pixbufscaled);
   init_halo_surface();
}

int do_umoon()
{
   static int xdirection = 1;
   static int ydirection = 1;
   if (Flags.Done)
      return FALSE;
   LEAVE_IF_INACTIVE;
   if(!Flags.Moon)
      return TRUE;

   moonX += xdirection*time_umoon*Flags.MoonSpeed/60.0;
   moonY += 0.2*ydirection*time_umoon*Flags.MoonSpeed/60.0;

   if (moonX > SnowWinWidth - 200 - Flags.MoonSize)
      xdirection = -1;
   else if (moonX < 200)
      xdirection = 1;

   if (moonY > 120)
      ydirection = -1;
   else if (moonY < 20)
      ydirection = 1;
   return TRUE;
}

void init_halo_surface()
{
   if (halo_surface)
      cairo_surface_destroy(halo_surface);
   cairo_pattern_t *pattern;
   moonR = Flags.MoonSize/2;
   haloR = 1.8*moonR;
   P("halo_draw %f %f \n",moonR,haloR);
   pattern = cairo_pattern_create_radial(haloR, haloR, moonR, haloR, haloR, haloR);  
   double bright = Flags.HaloBright * ALPHA * 0.01;
   //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 1.0, 1.0, 1.0, 0.4*ALPHA);
   //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 1.0, 1.0, 1.0, 0.0);
   //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 0.9725, 0.9725, 1.0, 0.4*ALPHA);
   //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 0.9725, 0.9725, 1.0, 0.0);
   //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 0.96078, 0.96078, 0.96078, 0.4*ALPHA);
   //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 0.96078, 0.96078, 0.96078, 0.0);
   //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 0.94, 0.94, 0.9, 0.4*ALPHA);
   //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 0.94, 0.94, 0.9, 0.0);
   //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 244.0/255, 241.0/255, 201.0/255, 0.4*ALPHA);
   //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 244.0/255, 241.0/255, 201.0/255, 0.0);
   //cairo_pattern_add_color_stop_rgba(pattern, 0.0, 245.0/255, 243.0/255, 206.0/255, 0.4*ALPHA);
   //cairo_pattern_add_color_stop_rgba(pattern, 1.0, 245.0/255, 243.0/255, 206.0/255, 0.0);
   cairo_pattern_add_color_stop_rgba(pattern, 0.0, 234.0/255, 244.0/255, 252.0/255, bright);
   cairo_pattern_add_color_stop_rgba(pattern, 1.0, 234.0/255, 244.0/255, 252.0/255, 0.0);

   GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 2*haloR, 2*haloR);
   halo_surface      = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2*haloR, 2*haloR);
   cairo_t *halocr   = cairo_create(halo_surface);
   gdk_cairo_set_source_pixbuf(halocr, pixbuf, 0, 0);

   cairo_set_source_rgba(halocr, 0, 0, 0, 0);
   cairo_paint          (halocr);
   cairo_set_source     (halocr, pattern);
   cairo_arc            (halocr, haloR, haloR, haloR, 0, M_PI * 2);
   cairo_fill           (halocr);     

   cairo_destroy          (halocr);
   cairo_pattern_destroy  (pattern);
   g_clear_object         (&pixbuf);
}

void halo_draw(cairo_t *cr)
{
   if (!Flags.Halo)
      return;
   P("halo_draw %f %f \n", moonR, haloR);
   double xc = moonX + moonR;
   double yc = moonY + moonR;

   cairo_set_source_surface(cr, halo_surface, xc-haloR, yc-haloR);
   my_cairo_paint_with_alpha(cr, ALPHA);
}
