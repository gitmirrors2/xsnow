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
#ifndef XSNOW_H
#define XSNOW_H

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "x11cairo.h"

#define SOMENUMBER 42

#define DEFAULT_below 1
#define DEFAULT_bgcolor "#000000"
#define DEFAULT_blowofffactor 100
#define DEFAULT_cpuload 100
#define DEFAULT_defaults 0
#define DEFAULT_desired_number_of_trees 10
#define DEFAULT_desktop 0
#define DEFAULT_display_name ""
#define DEFAULT_done 0
#define DEFAULT_exposures -SOMENUMBER
#define DEFAULT_flakecountmax 10000     // more flakes leads to a broken fuse
#define DEFAULT_fullscreen 0
#define DEFAULT_KDEbg 0
#define DEFAULT_maxontrees 200
#define DEFAULT_MaxScrSnowDepth 50    /* Will build up to MAXSRCSNOWDEPTH */
#define DEFAULT_MaxWinSnowDepth 30  // wwvv
#define DEFAULT_NoBlowSnow 0
#define DEFAULT_noconfig 0
#define DEFAULT_NoFluffy 0
#define DEFAULT_NoKeepSBot 0
#define DEFAULT_NoKeepSnow 0
#define DEFAULT_NoKeepSnowOnTrees 0
#define DEFAULT_NoKeepSWin 0
#define DEFAULT_nomenu 0
#define DEFAULT_NoMeteorites 0
#define DEFAULT_NoRudolf 0 
#define DEFAULT_NoSanta 0
#define DEFAULT_NoSnowFlakes 0
#define DEFAULT_NoTrees 0
#define DEFAULT_NoWind 0
#define DEFAULT_nstars 20
#define DEFAULT_offset_s 0 
#define DEFAULT_offset_w -8
#define DEFAULT_offset_x 4
#define DEFAULT_offset_y 0 
#define DEFAULT_quiet 1
#define DEFAULT_SantaSize 3         // default santa size    
#define DEFAULT_SantaSpeedFactor 100
#define DEFAULT_showstats 0 
#define DEFAULT_snowColor "snow"
#define DEFAULT_snowflakesfactor 100
#define DEFAULT_SnowSpeedFactor 100
#define DEFAULT_stopafter -1
#define DEFAULT_trColor "chartreuse" // The author thoroughly recommends a cup of tea with a dash of green Chartreuse. Jum!         */
#define DEFAULT_treefill 30
#define DEFAULT_TreeType "1,2,3,4,5,6,"       // default treetype
#define DEFAULT_usealpha  SOMENUMBER
#define DEFAULT_usebg 0 
#define DEFAULT_WhirlFactor 100
#define DEFAULT_windnow 0
#define DEFAULT_window_id 0
#define DEFAULT_WindTimer 30
#define DEFAULT_xwininfohandling 0
#define DEFAULTTREETYPE 2
#define ALLTREETYPES "0" DEFAULT_TreeType

#define VINTAGE_desired_number_of_trees 6
#define VINTAGE_NoBlowSnow 1
#define VINTAGE_NoKeepSnowOnTrees 1
#define VINTAGE_NoMeteorites 1
#define VINTAGE_NoRudolf 0 
#define VINTAGE_nstars 0
#define VINTAGE_SantaSize 2 
#define VINTAGE_snowflakesfactor 15
#define VINTAGE_TreeType "0,"

#define FLAGSFILE ".xsnowrc"
#define FLAKES_PER_SEC_PER_PIXEL 30
#define INITIALSCRPAINTSNOWDEPTH 8  /* Painted in advance */
#define INITIALYSPEED 120   // has to do with vertical flake speed
#define MAXBLOWOFFFACTOR 10
#define MAXSANTA	4    // santa types 0..4
#define MAXTANNENPLACES 10   // number of trees
#define MAXTREETYPE 6        // treetypes: 0..MAXTREETYPE
#define MAXWSENS 0.2        // sensibility of flakes for wind
#define MAXXSTEP 2             /* drift speed max */
#define MAXYSTEP 10             /* falling speed max */
#define PIXINANIMATION	4    // nr of santa animations 
#define SANTASENS 0.2       // sensibility of Santa for wind
#define SANTASPEED0 12
#define SANTASPEED1 25
#define SANTASPEED2 50
#define SANTASPEED3 50
#define SANTASPEED4 70
#define SNOWFLAKEMAXTYPE 6  
#define SNOWFREE 25  /* Must stay snowfree on display :) */
#define STARANIMATIONS 4
#define WHIRL 150


/* ------------------------------------------------------------------ */


typedef struct SnowMap {
   char *snowBits;
   Pixmap pixmap;
   REGION r;
   int width;
   int height;
#ifndef USEX11
   cairo_surface_t *s;
   GdkPixbuf *pixbuf;
#endif
} SnowMap;


// star stuff


typedef struct StarMap {
   unsigned char *starBits;
   Pixmap pixmap;
   int width;
   int height;
} StarMap;

typedef struct Skoordinaten {
   int x; 
   int y; 
   int color; 
} Skoordinaten;

// meteorites stuff

typedef struct MeteoMap {
   int x1,x2,y1,y2,active;
   double starttime;
   GC gc,egc;
   REGION r;
} MeteoMap;

/* ------------------------------------------------------------------ */


typedef struct TannenbaumMap {
   char *tannenbaumBits;
   Pixmap pixmap;
   int width;
   int height;
} TannenbaumMap;


typedef struct Treeinfo { 
   int x;                    // x position
   int y;                    // y position
   unsigned int type;        // type (TreeType, -treetype)
   unsigned int rev:1;       // reversed
} Treeinfo;

/* ------------------------------------------------------------------ */

#endif
