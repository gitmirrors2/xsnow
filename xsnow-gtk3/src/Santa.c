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
#include "Santa.h"
#include "pixmaps.h"
#include "debug.h"
#include "windows.h"

static void init_Santa_surfaces(void);

cairo_surface_t *Santa_surfaces[MAXSANTA+1][2][PIXINANIMATION];

int Santa_draw(cairo_t *cr)
{
   cairo_surface_t *surface;
   static int a = 0;
   a++;
   if (a > 3)
      a=0;
   static int x = 0;
   x+=5;
   if (x>1000)
      x = 0;
   surface = Santa_surfaces[3][1][a];
   cairo_set_source_surface (cr, surface, x,300);
   cairo_paint(cr);
   return TRUE;
}

void Santa_init()
{
   init_Santa_surfaces();
}

void init_Santa_surfaces()
{
   GdkPixbuf *pixbuf;
   int i,j,k;
   for(i=0; i<MAXSANTA+1; i++)
      for (j=0; j<2; j++)
	 for (k=0; k<PIXINANIMATION; k++)
	 {
	    R("%d %d %d\n",i,j,k);
	    pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)Santas[i][j][k]);
	    Santa_surfaces[i][j][k] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, gdkwindow);
	    g_clear_object(&pixbuf);
	 }
}
