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
#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>



#define FLAGSFILE ".xsnowrc"
#define FLAKES_PER_SEC_PER_PIXEL 30
#define INITIALSCRPAINTSNOWDEPTH 8  /* Painted in advance */
#define INITIALYSPEED 120   // has to do with vertical flake speed
#define MAXBLOWOFFFACTOR 100
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
//#define SNOWFLAKEMAXTYPE 13  // type is 0..SNOWFLAKEMAXTYPE  
#define SNOWFREE 25  /* Must stay snowfree on display :) */
#define SNOWSPEED 0.7    // the higher, the speedier the snow
#define WHIRL 150


// timers

#define time_blowoff          0.50             /* time between blow snow off windows         */
#define time_clean            1.00             /* time between cleaning desktop              */
#define time_displaychanged   1.00             /* time between checks if display has changed */
#define time_emeteorite       0.20             /* time between meteorites erasures           */ 
#define time_event            0.01             /* time between checking events               */
#define time_flakecount       1.00             /* time between updates of show flakecount    */
#define time_fuse             1.00             /* time between testing on too much flakes    */
#define time_genflakes        0.10             /* time between generation of flakes          */
#define time_initbaum         1.00             /* time between check for (re)create trees    */
#define time_initstars        1.00             /* time between check for (re)create stars    */
#define time_meteorite        3.00             /* time between meteorites                    */
#define time_newwind          1.00             /* time between changing wind                 */
#define time_sfallen          2.30             /* time between smoothing of fallen snow      */
#define time_snow_on_trees    0.50             /* time between redrawings of snow on trees   */
#define time_star             0.50             /* time between drawing stars                 */ 
#define time_testing          2.10             /* time between testing code                  */ 
#define time_ui_check         0.25             /* time between checking values from ui       */ 
#define time_umoon            0.02            /* time between update position of moon       */
#define time_usanta           0.02             /* time between update of santa position      */
#define time_ustar            2.00             /* time between updating stars                */ 
#define time_wind             0.10             /* time between starting or ending wind       */
#define time_wupdate          0.40             /* time between getting windows information   */ 
#define time_show_range_etc   0.50             /* time between showing range etc.            */
#define time_change_attr      60.0             /* time between changing attraction point     */
#define time_measure          0.1

#define time_fallen           (0.04 * cpufactor)  /* time between redraw fallen snow            */
#define time_santa            (0.02 * cpufactor)  /* time between drawings of santa             */
#define time_santa1           (0.01 * cpufactor)  /* time between redrawings of santa           */
//#define time_snowflakes       (0.05 * cpufactor)  /* time between redrawings of snowflakes      */
#define time_snowflakes       (0.02 * cpufactor)  /* time between redrawings of snowflakes      */
//#define time_snowflakes       (switches.UseGtk?0.02 * cpufactor:0.05*cpufactor)  /* time between redrawings of snowflakes      */
#define time_tree             (0.25 * cpufactor)  /* time between redrawings of trees           */

#define time_draw_all         (0.04 * cpufactor)    /* time between updates of screen */

/* ------------------------------------------------------------------ */

extern double cpufactor;

