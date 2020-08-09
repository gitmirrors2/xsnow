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
#include "debug.h"
#include "xsnow.h"
#include "pixmaps.h"
#include "windows.h"
#include "hashtable.h"
#include "flags.h"
#include "ixpm.h"
#include "snow.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

static cairo_surface_t *snow_surfaces[SNOWFLAKEMAXTYPE+1];

//static void init_snow_surfaces(void);

void snow_init()
{
   int i;
   for (i=0; i<SNOWFLAKEMAXTYPE +1; i++)
      snow_surfaces[i] = 0;
   init_snow_surfaces();
}

void init_snow_surfaces()
{
   GdkPixbuf *pixbuf;
   int i;
   for(i=0; i<SNOWFLAKEMAXTYPE+1; i++)
   {
      R("%d\n",i);
      char **x;
      int lines;
      xpm_set_color(snow_xpm[i], &x, &lines, Flags.SnowColor);
      pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)x);
      xpm_destroy(x,lines);
      if (snow_surfaces[i])
	 cairo_surface_destroy(snow_surfaces[i]);
      snow_surfaces[i] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, gdkwindow);
      g_clear_object(&pixbuf);
   }
}

int snow_draw(cairo_t *cr)
{
   if (Flags.Done)
      return FALSE;
   if (Flags.NoSnowFlakes || NOTACTIVE)
      return TRUE;

   set_begin();
   Snow *flake;
   while( (flake = set_next()) )
   {
      cairo_surface_t *surface;
      surface = snow_surfaces[flake->whatFlake];
      cairo_set_source_surface (cr, surface, flake->rx, flake->ry);
      cairo_paint(cr);
   }
   return TRUE;
}

