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
#include <math.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "blowoff.h"
#include "utils.h"
#include "xsnow.h"
#include "flags.h"
#include "debug.h"
#include "windows.h"
#include "fallensnow.h"
#include "varia.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

static float BlowOffFactor;

void blowoff_init()
{
   add_to_mainloop(PRIORITY_DEFAULT, time_blowoff, do_blowoff, NULL);
}

int blowoff_ui()
{
   int changes = 0;
   if(Flags.BlowOffFactor != OldFlags.BlowOffFactor)
   {
      OldFlags.BlowOffFactor = Flags.BlowOffFactor;
      InitBlowOffFactor();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoBlowSnow != OldFlags.NoBlowSnow)
   {
      OldFlags.NoBlowSnow = Flags.NoBlowSnow;
      changes++;
      P("changes: %d\n",changes);
   }
   return changes;
}

void blowoff_draw(UNUSED cairo_t *cr)
{
   // nothing to draw here
}

int BlowOff()
{
   float g = gaussian(BlowOffFactor,0.5*BlowOffFactor,0.0,2.0*MAXBLOWOFFFACTOR);
   return lrint(g);
}

void InitBlowOffFactor()
{
   BlowOffFactor = 0.01*Flags.BlowOffFactor;
   if (BlowOffFactor > MAXBLOWOFFFACTOR)
      BlowOffFactor = MAXBLOWOFFFACTOR;
}

// determine if fallensnow should be handled for fsnow
int do_blowoff(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE || Flags.NoBlowSnow)
      return TRUE;
   FallenSnow *fsnow = FsnowFirst;
   while(fsnow)
   {
      P("blowoff ...\n");
      if (HandleFallenSnow(fsnow)) 
	 if(fsnow->win.id == 0 || (!fsnow->win.hidden &&
		  (fsnow->win.ws == CWorkSpace || fsnow->win.sticky)))
	    UpdateFallenSnowWithWind(fsnow,fsnow->w/4,fsnow->h/4); 
      fsnow = fsnow->next;
   }
   return TRUE;
}

