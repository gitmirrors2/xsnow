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
#ifndef PIXMAPS_H
#define PIXMAPS_H
#include "xsnow.h"
extern SnowMap       snowPix[SNOWFLAKEMAXTYPE+1];
extern StarMap       starPix;
extern char          ***Santas[MAXSANTA+1][2];
extern char          **xpmtrees[MAXTREETYPE+1];
//extern TannenbaumMap tannenbaumPix[];
extern char          **xsnow_logo;
extern char          **Snows[SNOWFLAKEMAXTYPE+1];
void surface_change_color(cairo_surface_t *s, GdkRGBA *c);
#endif
