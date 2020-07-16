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
#include "xsnow.h"
#include "pixmaps.h"

#include "Pixmaps/snow00.xbm"
#include "Pixmaps/snow01.xbm"
#include "Pixmaps/snow02.xbm"
#include "Pixmaps/snow03.xbm"
#include "Pixmaps/snow04.xbm"
#include "Pixmaps/snow05.xbm"
#include "Pixmaps/snow06.xbm"

SnowMap snowPix[] = {
   {snow00_bits, None, snow00_width, snow00_height},
   {snow01_bits, None, snow01_width, snow01_height},
   {snow02_bits, None, snow02_width, snow02_height},
   {snow03_bits, None, snow03_width, snow03_height},
   {snow04_bits, None, snow04_width, snow04_height},
   {snow05_bits, None, snow05_width, snow05_height},
   {snow06_bits, None, snow06_width, snow06_height},
}; 

#include "Pixmaps/BigSanta1.xpm"
#include "Pixmaps/BigSanta2.xpm"
#include "Pixmaps/BigSanta3.xpm"
#include "Pixmaps/BigSanta4.xpm"

static char **BigSanta[] = 
{(char **)BigSanta1,(char **)BigSanta2,(char **)BigSanta3,(char **)BigSanta4};

#include "Pixmaps/BigSantaRudolf1.xpm"
#include "Pixmaps/BigSantaRudolf2.xpm"
#include "Pixmaps/BigSantaRudolf3.xpm"
#include "Pixmaps/BigSantaRudolf4.xpm"

static char **BigSantaRudolf[] =
{(char **)BigSantaRudolf1,(char **)BigSantaRudolf2,(char **)BigSantaRudolf3,(char **)BigSantaRudolf4};

#include "Pixmaps/BigSanta81.xpm"
#include "Pixmaps/BigSanta82.xpm"
#include "Pixmaps/BigSanta83.xpm"
#include "Pixmaps/BigSanta84.xpm"

static char **BigSanta8[] =
{(char **)BigSanta81,(char **)BigSanta82,(char **)BigSanta83,(char **)BigSanta84};

#include "Pixmaps/BigSantaRudolf81.xpm"
#include "Pixmaps/BigSantaRudolf82.xpm"
#include "Pixmaps/BigSantaRudolf83.xpm"
#include "Pixmaps/BigSantaRudolf84.xpm"

static char **BigSantaRudolf8[] =
{(char **)BigSantaRudolf81,(char **)BigSantaRudolf82,(char **)BigSantaRudolf83,(char **)BigSantaRudolf84};

#include "Pixmaps/MediumSanta1.xpm"
#include "Pixmaps/MediumSanta2.xpm"
#include "Pixmaps/MediumSanta3.xpm"
#include "Pixmaps/MediumSanta4.xpm"

static char **MediumSanta[] =
{(char **)MediumSanta1,(char **)MediumSanta2,(char **)MediumSanta3,(char **)MediumSanta4};

#include "Pixmaps/MediumSantaRudolf1.xpm"
#include "Pixmaps/MediumSantaRudolf2.xpm"
#include "Pixmaps/MediumSantaRudolf3.xpm"
#include "Pixmaps/MediumSantaRudolf4.xpm"

static char **MediumSantaRudolf[] =
{(char **)MediumSantaRudolf1,(char **)MediumSantaRudolf2,(char **)MediumSantaRudolf3,(char **)MediumSantaRudolf4};

#include "Pixmaps/RegularSanta1.xpm"
#include "Pixmaps/RegularSanta2.xpm"
#include "Pixmaps/RegularSanta3.xpm"
#include "Pixmaps/RegularSanta4.xpm"

static char **RegularSanta[] = 
{(char **)RegularSanta1,(char **)RegularSanta2,(char **)RegularSanta3,(char **)RegularSanta4};

#include "Pixmaps/RegularSantaRudolf1.xpm"
#include "Pixmaps/RegularSantaRudolf2.xpm"
#include "Pixmaps/RegularSantaRudolf3.xpm"
#include "Pixmaps/RegularSantaRudolf4.xpm"

static char **RegularSantaRudolf[] =
{(char **)RegularSantaRudolf1,(char **)RegularSantaRudolf2,(char **)RegularSantaRudolf3,(char **)RegularSantaRudolf4};

#include "Pixmaps/AltSanta1.xpm"
#include "Pixmaps/AltSanta2.xpm"
#include "Pixmaps/AltSanta3.xpm"
#include "Pixmaps/AltSanta4.xpm"

static char **AltSanta[] =
{(char **)AltSanta1,(char **)AltSanta2,(char **)AltSanta3,(char **)AltSanta4};

#include "Pixmaps/AltSantaRudolf1.xpm"
#include "Pixmaps/AltSantaRudolf2.xpm"
#include "Pixmaps/AltSantaRudolf3.xpm"
#include "Pixmaps/AltSantaRudolf4.xpm"

static char **AltSantaRudolf[] =
{(char **)AltSantaRudolf1,(char **)AltSantaRudolf2,(char **)AltSantaRudolf3,(char **)AltSantaRudolf4};

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
#include "Pixmaps/polarbear.xpm"

char **xpmtrees[MAXTREETYPE+1] =
{(char **)tannenbaum_xpm, (char **)tree_xpm, (char **)tree_1_100px, (char **)huis4_xpm, 
   (char **)reindeer_xpm, (char **)eland_xpm, (char **)snowtree_xpm, (char **)polarbear_xpm};

#include "Pixmaps/star.xbm"

StarMap starPix =
{ star_bits, None, star_height, star_width };

#include "Pixmaps/xsnow.xpm"
char **xsnow_logo = (char **)xsnow_xpm;

/* front bird */
#include "Pixmaps/bird1.xpm"
#include "Pixmaps/bird2.xpm"
#include "Pixmaps/bird3.xpm"
#include "Pixmaps/bird4.xpm"
#include "Pixmaps/bird5.xpm"
#include "Pixmaps/bird6.xpm"
#include "Pixmaps/bird7.xpm"
#include "Pixmaps/bird8.xpm"

/* side bird */
#include "Pixmaps/birdl1.xpm"
#include "Pixmaps/birdl2.xpm"
#include "Pixmaps/birdl3.xpm"
#include "Pixmaps/birdl4.xpm"
#include "Pixmaps/birdl5.xpm"
#include "Pixmaps/birdl6.xpm"
#include "Pixmaps/birdl7.xpm"
#include "Pixmaps/birdl8.xpm"

/* oblique bird */
#include "Pixmaps/birdd1.xpm"
#include "Pixmaps/birdd2.xpm"
#include "Pixmaps/birdd3.xpm"
#include "Pixmaps/birdd4.xpm"
#include "Pixmaps/birdd5.xpm"
#include "Pixmaps/birdd6.xpm"
#include "Pixmaps/birdd7.xpm"
#include "Pixmaps/birdd8.xpm"

const char **birds_xpm[] = 
{bird1_xpm,bird2_xpm,bird3_xpm,bird4_xpm,bird5_xpm,bird6_xpm,bird7_xpm,bird8_xpm,
   birdl1_xpm,birdl2_xpm,birdl3_xpm,birdl4_xpm,birdl5_xpm,birdl6_xpm,birdl7_xpm,birdl8_xpm,
   birdd1_xpm,birdd2_xpm,birdd3_xpm,birdd4_xpm,birdd5_xpm,birdd6_xpm,birdd7_xpm,birdd8_xpm};

