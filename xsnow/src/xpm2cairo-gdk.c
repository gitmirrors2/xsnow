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
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <X11/xpm.h>
#include "xpm2cairo-gdk.h"
/*
   converts xpm-data or xpm-file to a cairo_surface_t
xpm: xpm-data
fname: file name
if fname == NULL, xpm is used
*/
cairo_surface_t *xpm2cairo(Display *dpy, char** xpm, char* fname)
{

   Window window = DefaultRootWindow(dpy);
   Pixmap pixmap, shapemask;
   XpmAttributes attributes;
   attributes.valuemask = 0;
   if(fname)
      XpmReadFileToPixmap(dpy, window, fname, 
	    &pixmap, &shapemask, &attributes);
   else
      XpmCreatePixmapFromData(dpy, window, xpm, 
	    &pixmap, &shapemask, &attributes);

   unsigned int width, height;
   width = attributes.width;
   height = attributes.height;

   // create xpm_surface from pixmap:
   Visual *visual = DefaultVisual(dpy,0);
   cairo_surface_t *xpm_surface = cairo_xlib_surface_create(dpy,
	 pixmap, visual, width, height); 

   // create return_surface from xpm_surface:
   cairo_surface_t *return_surface = cairo_surface_create_similar_image(xpm_surface,
	 CAIRO_FORMAT_ARGB32, width, height);

   cairo_t *cr = cairo_create(return_surface);

   // shapemask is NULL if there is no transparency in the xpm.
   if (shapemask)
   {
      // transparency, use shapemask as mask
      cairo_surface_t *shape_surface;
      Screen *screen = DefaultScreenOfDisplay(dpy);
      shape_surface = cairo_xlib_surface_create_for_bitmap(dpy,
	    shapemask, screen, width, height); 

      // convert shape_surface (which is CAIRO_FORMAT_A1) to a 32 bit surface for return:
      cairo_set_source_surface(cr, shape_surface, 0, 0);
      cairo_paint(cr);
      XFreePixmap(dpy, shapemask);
      cairo_surface_destroy(shape_surface);
      // apply masking operation:
      cairo_set_operator(cr, CAIRO_OPERATOR_IN);
   }

   cairo_set_source_surface(cr, xpm_surface, 0, 0);
   cairo_paint(cr);

   cairo_destroy(cr);
   cairo_surface_destroy(xpm_surface);
   XFreePixmap(dpy, pixmap);
   XDestroyWindow(dpy,window);

   return return_surface;
}

/*
   converts xpm-data or xpm-file to a GdkPixbuf
xpm: xpm-data
fname: file name
if fname == NULL, xpm is used
*/
GdkPixbuf *xpm2gdk(Display *dpy, char** xpm, char *fname)
{
#ifdef USE_GDK_PIXBUF_NEW_FROM_XPM_DATA
   // use deprecated gdk_pixbuf_new_from_xpm_data
   (void)dpy;    // unused
   (void)fname;  // unused
   return gdk_pixbuf_new_from_xpm_data((const char**)xpm);
#else
   cairo_surface_t *surface = xpm2cairo(dpy, xpm, fname); 
   GdkPixbuf *gdkpixbuf     = gdk_pixbuf_get_from_surface (
	 surface,
	 0,
	 0,
	 cairo_image_surface_get_width(surface),
	 cairo_image_surface_get_height(surface)
	 );
   cairo_surface_destroy(surface);
   return gdkpixbuf;
#endif
}

