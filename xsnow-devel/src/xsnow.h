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
#ifndef XSNOW_H
#define XSNOW_H
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define SOMENUMBER 42

#define DEFAULT_AllWorkspaces 1
#define DEFAULT_BelowAll 1
#define DEFAULT_BGColor "#000000"
#define DEFAULT_BlowOffFactor 100
#define DEFAULT_CpuLoad 100
#define DEFAULT_Defaults 0
#define DEFAULT_DesiredNumberOfTrees 10
#define DEFAULT_Desktop 0
#define DEFAULT_DisplayName ""
#define DEFAULT_Done 0
#define DEFAULT_Exposures -SOMENUMBER
#define DEFAULT_FlakeCountMax 1000 
#define DEFAULT_FullScreen 0
#define DEFAULT_KDEbg 0
#define DEFAULT_MaxOnTrees 200
#define DEFAULT_MaxScrSnowDepth 50    /* Will build up to MAXSRCSNOWDEPTH */
#define DEFAULT_MaxWinSnowDepth 30  // wwvv
#define DEFAULT_NoBlowSnow 0
#define DEFAULT_NoConfig 0
#define DEFAULT_NoFluffy 0
#define DEFAULT_NoKeepSBot 0
#define DEFAULT_NoKeepSnow 0
#define DEFAULT_NoKeepSnowOnTrees 0
#define DEFAULT_NoKeepSWin 0
#define DEFAULT_NoMenu 0
#define DEFAULT_NoMeteorites 0
#define DEFAULT_NoRudolf 0 
#define DEFAULT_NoSanta 0
#define DEFAULT_NoSnowFlakes 0
#define DEFAULT_NoTrees 0
#define DEFAULT_NoWind 0
#define DEFAULT_NStars 20
#define DEFAULT_OffsetS 0 
#define DEFAULT_OffsetW -8
#define DEFAULT_OffsetX 4
#define DEFAULT_OffsetY 0 
#define DEFAULT_Quiet 1
#define DEFAULT_SantaSize 3         // default santa size    
#define DEFAULT_SantaSpeedFactor 100
#define DEFAULT_ShowStats 0 
#define DEFAULT_SnowColor "snow"
#define DEFAULT_SnowFlakesFactor 100
#define DEFAULT_SnowSpeedFactor 100
#define DEFAULT_StopAfter -1
#define DEFAULT_TreeColor "chartreuse" // The author thoroughly recommends a cup of tea with a dash of green Chartreuse. Jum!         */
#define DEFAULT_TreeFill 30
#define DEFAULT_TreeType "1,2,3,4,5,6,7,"       // default treetype
#define DEFAULT_UseAlpha  SOMENUMBER
#define DEFAULT_UseBG 0 
#define DEFAULT_WhirlFactor 100
#define DEFAULT_WindNow 0
#define DEFAULT_WindowId 0
#define DEFAULT_WindTimer 30
#define DEFAULT_XWinInfoHandling 0
#define DEFAULTTREETYPE 2
#define ALLTREETYPES "0" DEFAULT_TreeType

#define VINTAGE_DesiredNumberOfTrees 6
#define VINTAGE_NoBlowSnow 1
#define VINTAGE_NoKeepSnowOnTrees 1
#define VINTAGE_NoMeteorites 1
#define VINTAGE_NoRudolf 0 
#define VINTAGE_NStars 0
#define VINTAGE_SantaSize 2 
#define VINTAGE_SnowFlakesFactor 15
#define VINTAGE_TreeType "0,"

#define FLAGSFILE ".xsnowrc"
#define FLAKES_PER_SEC_PER_PIXEL 30
#define INITIALSCRPAINTSNOWDEPTH 8  /* Painted in advance */
#define INITIALYSPEED 120   // has to do with vertical flake speed
#define MAXBLOWOFFFACTOR 10
#define MAXSANTA	4    // santa types 0..4
#define MAXTANNENPLACES 10   // number of trees
#define MAXTREETYPE 7        // treetypes: 0..MAXTREETYPE
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
#define SNOWSPEED 0.7    // the higher, the speedier the snow
#define STARANIMATIONS 4
#define WHIRL 150


// timers

#define time_blowoff          0.50             /* time between blow snow off windows         */
#define time_clean            1.00             /* time between cleaning desktop              */
#define time_displaychanged   1.00             /* time between checks if display has changed */
#define time_emeteorite       0.20             /* time between meteorites erasures           */ 
#define time_event            0.01             /* time between checking events               */
#define time_fallen           (0.15 * factor)  /* time between redraw fallen snow            */
#define time_fuse             1.00             /* time between testing on too much flakes    */
#define time_genflakes        0.10             /* time between generation of flakes          */
#define time_meteorite        3.00             /* time between meteorites                    */
#define time_newwind          1.00             /* time between changing wind                 */
#define time_santa            (0.02 * factor)  /* time between drawings of santa             */
#define time_santa1           (0.01 * factor)  /* time between redrawings of santa           */
#define time_sfallen          2.30             /* time between smoothing of fallen snow      */
#define time_snow_on_trees    0.50             /* time between redrawings of snow on trees   */
#define time_snowflakes       (0.05 * factor)  /* time between redrawings of snow on trees   */
#define time_stars            0.50             /* time between drawing stars                 */ 
#define time_testing          1.05             /* time between testing code                  */ 
#define time_tree             (0.05 * factor)  /* time between redrawings of trees           */
#define time_ui_check         0.25             /* time between checking values from ui       */ 
#define time_usanta           0.02             /* time between update of santa position      */
#define time_ustars           2.00             /* time between updating stars                */ 
#define time_wind             0.10             /* time between starting or ending wind       */
#define time_wupdate          0.50             /* time between getting windows information   */ 

/* ------------------------------------------------------------------ */

typedef struct Snow {
   int w;                   // width
   int h;                   // height
   float rx;                // x position
   float ry;                // y position
   float vx;                // speed in x-direction, pixels/second
   float vy;                // speed in y-direction, pixels/second
   float m;                 // mass of flake
   float ivy;               // initial speed in y direction
   float wsens;             // wind dependency factor
   unsigned int cyclic : 1; // 0: flake is not cyclic 
   int whatFlake;           // snowflake index
} Snow;

typedef struct SnowMap {
   char *snowBits;
   Pixmap pixmap;
   int width;
   int height;
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
   Region r;
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
