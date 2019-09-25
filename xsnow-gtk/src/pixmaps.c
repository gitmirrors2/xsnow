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
#include "xsnow.h"
#include "pixmaps.h"

#include "Pixmaps/snow00.xbm"
#include "Pixmaps/snow01.xbm"
#include "Pixmaps/snow02.xbm"
#include "Pixmaps/snow03.xbm"
#include "Pixmaps/snow04.xbm"
#include "Pixmaps/snow05.xbm"
#include "Pixmaps/snow06.xbm"

SnowMap snowPix[SNOWFLAKEMAXTYPE+1] = {
   {snow00_bits, None, 0, snow00_width, snow00_height},
   {snow01_bits, None, 0, snow01_width, snow01_height},
   {snow02_bits, None, 0, snow02_width, snow02_height},
   {snow03_bits, None, 0, snow03_width, snow03_height},
   {snow04_bits, None, 0, snow04_width, snow04_height},
   {snow05_bits, None, 0, snow05_width, snow05_height},
   {snow06_bits, None, 0, snow06_width, snow06_height},
}; 

#include "Pixmaps/snow00.xpm"
#include "Pixmaps/snow01.xpm"
#include "Pixmaps/snow02.xpm"
#include "Pixmaps/snow03.xpm"
#include "Pixmaps/snow04.xpm"
#include "Pixmaps/snow05.xpm"
#include "Pixmaps/snow06.xpm"

char **Snows[SNOWFLAKEMAXTYPE+1] =
{snow00,snow01,snow02,snow03,snow04,snow05,snow06};

#include "Pixmaps/BigSanta1.xpm"
#include "Pixmaps/BigSanta2.xpm"
#include "Pixmaps/BigSanta3.xpm"
#include "Pixmaps/BigSanta4.xpm"

static char **BigSanta[PIXINANIMATION] = 
{BigSanta1,BigSanta2,BigSanta3,BigSanta4};

#include "Pixmaps/BigSantaRudolf1.xpm"
#include "Pixmaps/BigSantaRudolf2.xpm"
#include "Pixmaps/BigSantaRudolf3.xpm"
#include "Pixmaps/BigSantaRudolf4.xpm"

static char **BigSantaRudolf[PIXINANIMATION] =
{BigSantaRudolf1,BigSantaRudolf2,BigSantaRudolf3,BigSantaRudolf4};

#include "Pixmaps/BigSanta81.xpm"
#include "Pixmaps/BigSanta82.xpm"
#include "Pixmaps/BigSanta83.xpm"
#include "Pixmaps/BigSanta84.xpm"

static char **BigSanta8[PIXINANIMATION] =
{BigSanta81,BigSanta82,BigSanta83,BigSanta84};

#include "Pixmaps/BigSantaRudolf81.xpm"
#include "Pixmaps/BigSantaRudolf82.xpm"
#include "Pixmaps/BigSantaRudolf83.xpm"
#include "Pixmaps/BigSantaRudolf84.xpm"

static char **BigSantaRudolf8[PIXINANIMATION] =
{BigSantaRudolf81,BigSantaRudolf82,BigSantaRudolf83,BigSantaRudolf84};

#include "Pixmaps/MediumSanta1.xpm"
#include "Pixmaps/MediumSanta2.xpm"
#include "Pixmaps/MediumSanta3.xpm"
#include "Pixmaps/MediumSanta4.xpm"

static char **MediumSanta[PIXINANIMATION] =
{MediumSanta1,MediumSanta2,MediumSanta3,MediumSanta4};

#include "Pixmaps/MediumSantaRudolf1.xpm"
#include "Pixmaps/MediumSantaRudolf2.xpm"
#include "Pixmaps/MediumSantaRudolf3.xpm"
#include "Pixmaps/MediumSantaRudolf4.xpm"

static char **MediumSantaRudolf[PIXINANIMATION] =
{MediumSantaRudolf1,MediumSantaRudolf2,MediumSantaRudolf3,MediumSantaRudolf4};

#include "Pixmaps/RegularSanta1.xpm"
#include "Pixmaps/RegularSanta2.xpm"
#include "Pixmaps/RegularSanta3.xpm"
#include "Pixmaps/RegularSanta4.xpm"

static char **RegularSanta[PIXINANIMATION] = 
{RegularSanta1,RegularSanta2,RegularSanta3,RegularSanta4};

#include "Pixmaps/RegularSantaRudolf1.xpm"
#include "Pixmaps/RegularSantaRudolf2.xpm"
#include "Pixmaps/RegularSantaRudolf3.xpm"
#include "Pixmaps/RegularSantaRudolf4.xpm"

static char **RegularSantaRudolf[PIXINANIMATION] =
{RegularSantaRudolf1,RegularSantaRudolf2,RegularSantaRudolf3,RegularSantaRudolf4};

#include "Pixmaps/AltSanta1.xpm"
#include "Pixmaps/AltSanta2.xpm"
#include "Pixmaps/AltSanta3.xpm"
#include "Pixmaps/AltSanta4.xpm"

static char **AltSanta[PIXINANIMATION] =
{AltSanta1,AltSanta2,AltSanta3,AltSanta4};

#include "Pixmaps/AltSantaRudolf1.xpm"
#include "Pixmaps/AltSantaRudolf2.xpm"
#include "Pixmaps/AltSantaRudolf3.xpm"
#include "Pixmaps/AltSantaRudolf4.xpm"

static char **AltSantaRudolf[PIXINANIMATION] =
{AltSantaRudolf1,AltSantaRudolf2,AltSantaRudolf3,AltSantaRudolf4};

char ***Santas[MAXSANTA+1][2] =
{
   { RegularSanta , RegularSantaRudolf },
   { MediumSanta  , MediumSantaRudolf  },
   { BigSanta     , BigSantaRudolf     },
   { BigSanta8    , BigSantaRudolf8    },
   { AltSanta     , AltSantaRudolf     }
};

// so: Santas[type][rudolf][animation]

#include "Pixmaps/tannenbaum.xpm"
#include "Pixmaps/tree.xpm"
#include "Pixmaps/tree-1_100px.xpm"
#include "Pixmaps/huis4.xpm"
#include "Pixmaps/rendier.xpm"
#include "Pixmaps/eland.xpm"
#include "Pixmaps/snowtree.xpm"

char **xpmtrees[MAXTREETYPE+1] =
{tannenbaum_xpm, tree_xpm, tree_1_100px, huis4_xpm, reindeer_xpm, eland_xpm, snowtree_xpm};

#include "Pixmaps/star.xbm"

StarMap starPix =
{ star_bits, None, star_height, star_width };

#include "Pixmaps/xsnow.xpm"
char **xsnow_logo = xsnow_xpm;

#include "Pixmaps/star.xpm"
char **star_xpm = starblack;

// changes pixels with alpha >0 into rgba
void surface_change_color(cairo_surface_t *s, GdkRGBA *c)
{
   int j,k,l = 0;
   unsigned int color = (int)(c->red*255) << 16 |
      (unsigned int)(255*c->green) << 8  | (unsigned int)(255*c->blue);
   cairo_surface_flush(s);
   unsigned int *p = (unsigned int *)cairo_image_surface_get_data(s);
   int height = cairo_image_surface_get_height(s);
   int stride = cairo_image_surface_get_stride(s);
   for (j=0; j<height; j++)
      for (k=0; k<stride/4; k++)
      {
	 if (p[l]& 0xff000000)
	    p[l] = color | (p[l] & 0xff000000);
	 l++;
      }
   cairo_surface_mark_dirty(s);
}
