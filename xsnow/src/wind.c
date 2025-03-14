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
#include <gtk/gtk.h>
#include <stdlib.h>


#include "xsnow-constants.h"

#include "wind.h"
#include "utils.h"
#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "clocks.h"
#include "xsnow.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())


static void   SetWhirl(void);
static void   SetWindTimer(void);
static int    do_wind(void *);
static int    do_newwind(void *);

void wind_init()
{
   SetWhirl();
   SetWindTimer();
   add_to_mainloop(PRIORITY_DEFAULT, time_newwind,        do_newwind );
   add_to_mainloop(PRIORITY_DEFAULT, time_wind,           do_wind    );
}

void wind_ui()
{
   UIDO(NoWind, global.Wind = 0; global.NewWind = 0;);
   UIDO(WhirlFactor, SetWhirl(););
   UIDO(WindTimer, SetWindTimer(););
   if(Flags.WindNow)
   {
      Flags.WindNow = 0;
      global.Wind = 2;
      P("Gust: %d\n",Flags.Changes);
   }
}

void draw_wind()
{
   // Nothing to draw
}

int do_newwind(void *d)
{
   P("newwind\n");
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   //
   // the speed of newwind is pixels/second
   // at steady Wind, eventually all flakes get this speed.
   //
   if(Flags.NoWind) return TRUE;
   static double t0 = -1;
   if (t0<0)
   {
      t0 = wallclock();
      return TRUE;
   }

   float r;
   switch (global.Wind)
   {
      case(0): 
      default:
	 r = drand48()*global.Whirl;
	 global.NewWind += r - global.Whirl/2;
	 if(global.NewWind > global.WindMax) global.NewWind = global.WindMax;
	 if(global.NewWind < -global.WindMax) global.NewWind = -global.WindMax;
	 break;
      case(1): 
	 global.NewWind = global.Direction*0.6*global.Whirl;
	 break;
      case(2):
	 global.NewWind = global.Direction*1.2*global.Whirl;
	 break;
   }
   return TRUE;
   (void)d;
}

int do_wind(void *d)
{
   P("wind\n");
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if(Flags.NoWind) return TRUE;
   static int first = 1;
   static double prevtime;

   double TNow = wallclock();
   if (first)
   {
      prevtime = TNow;;
      first    = 0;
   }

   // on the average, this function will do something
   // after WindTimer secs

   if ((TNow - prevtime) < 2*global.WindTimer*drand48()) return TRUE;

   prevtime = TNow;

   if(drand48() > 0.65)  // Now for some of Rick's magic:
   {
      if(drand48() > 0.4)
	 global.Direction = 1;
      else
	 global.Direction = -1;
      global.Wind = 2;
      global.WindTimer = 5;
      //               next time, this function will be active 
      //               after on average 5 secs
   }
   else
   {
      if(global.Wind == 2)
      {
	 global.Wind = 1;
	 global.WindTimer = 3;
	 //                   next time, this function will be active 
	 //                   after on average 3 secs
      }
      else
      {
	 global.Wind = 0;
	 global.WindTimer = global.WindTimerStart;
	 //                   next time, this function will be active 
	 //                   after on average WindTimerStart secs
      }
   }
   P("Wind: %d %f\n",global.Wind,global.NewWind);
   return TRUE;
   (void)d;
}

void SetWhirl()
{
   global.Whirl = 0.01*Flags.WhirlFactor*WHIRL;
}

void SetWindTimer()
{
   global.WindTimerStart    = Flags.WindTimer;
   if (global.WindTimerStart < 3) 
      global.WindTimerStart = 3;
   global.WindTimer         = global.WindTimerStart;
}
