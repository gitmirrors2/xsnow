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
#include <stdlib.h>

#include "wind.h"
#include "utils.h"
#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "clocks.h"
#include "xsnow.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

int Wind = 0;
int    Direction = 0;
double WindTimer;
double WindTimerStart;
float  Whirl;

static void   SetWhirl(void);
static void   SetWindTimer(void);
static int    do_wind(gpointer data);
static int    do_newwind(gpointer data);

void wind_init()
{
   SetWhirl();
   SetWindTimer();
   add_to_mainloop(PRIORITY_DEFAULT, time_newwind,        do_newwind            ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_wind,           do_wind               ,0);
}

int wind_ui()
{
   int changes = 0;
   if(Flags.NoWind != OldFlags.NoWind)
   {
      OldFlags.NoWind = Flags.NoWind;
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.WhirlFactor != OldFlags.WhirlFactor)
   {
      OldFlags.WhirlFactor = Flags.WhirlFactor;
      SetWhirl();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.WindTimer != OldFlags.WindTimer)
   {
      OldFlags.WindTimer = Flags.WindTimer;
      SetWindTimer();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.WindNow)
   {
      Flags.WindNow = 0;
      Wind = 2;
      P("changes: %d\n",changes);
   }
   return changes;
}

void draw_wind()
{
   // Nothing to draw
}

int do_newwind(gpointer data)
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

   float windmax = 100.0;
   float r;
   switch (Wind)
   {
      case(0): 
      default:
	 r = drand48()*Whirl;
	 NewWind += r - Whirl/2;
	 if(NewWind > windmax) NewWind = windmax;
	 if(NewWind < -windmax) NewWind = -windmax;
	 break;
      case(1): 
	 NewWind = Direction*0.6*Whirl;
	 break;
      case(2):
	 NewWind = Direction*1.2*Whirl;
	 break;
   }
   return TRUE;
}

int do_wind(gpointer data)
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

   if ((TNow - prevtime) < 2*WindTimer*drand48()) return TRUE;

   prevtime = TNow;

   if(drand48() > 0.65)  // Now for some of Rick's magic:
   {
      if(drand48() > 0.4)
	 Direction = 1;
      else
	 Direction = -1;
      Wind = 2;
      WindTimer = 5;
      //               next time, this function will be active 
      //               after on average 5 secs
   }
   else
   {
      if(Wind == 2)
      {
	 Wind = 1;
	 WindTimer = 3;
	 //                   next time, this function will be active 
	 //                   after on average 3 secs
      }
      else
      {
	 Wind = 0;
	 WindTimer = WindTimerStart;
	 //                   next time, this function will be active 
	 //                   after on average WindTimerStart secs
      }
   }
   return TRUE;
}
void SetWhirl()
{
   Whirl = 0.01*Flags.WhirlFactor*WHIRL;
}
void SetWindTimer()
{
   WindTimerStart           = Flags.WindTimer;
   if (WindTimerStart < 3) 
      WindTimerStart = 3;
   WindTimer                = WindTimerStart;
}
