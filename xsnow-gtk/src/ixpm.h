/* -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019 Willem Vermin
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
#ifndef IXPM_H
#define IXPM_H
#include <X11/xpm.h>
extern int iXpmCreatePixmapFromData(Display *display, Drawable d, 
      char **data, Pixmap *p,Pixmap *s, XpmAttributes *attr, int flop);
cairo_surface_t *igdk_cairo_surface_create_from_xpm(char *data[], int flop);
#ifdef USEX11
extern Region regionfromxpm(char **data, int flop);
#else
extern cairo_region_t *regionfromxpm(char **data, int flop);
#endif
#endif
