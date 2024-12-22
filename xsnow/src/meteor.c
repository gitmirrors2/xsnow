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

#include <pthread.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <stdlib.h>

#include "xsnow-constants.h"

#include "debug.h"
#include "flags.h"
#include "windows.h"
#include "clocks.h"
#include "snow.h"
#include "meteor.h"
#include "ui.h"
#include "utils.h"
#include "xsnow.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

static int do_emeteor(gpointer data);
static int do_meteor(void *);

#define NUMCOLORS 5
static GdkRGBA       colors[NUMCOLORS];
static MeteorMap     meteor;

static int           drawcount;

void meteor_init()
{
   meteor.x1       = 0;
   meteor.x2       = 0;
   meteor.y1       = 0;
   meteor.y2       = 0;
   meteor.active   = 0;
   meteor.colornum = 0;

   gdk_rgba_parse(&colors[0],"#f0e0e0");
   gdk_rgba_parse(&colors[1],"#e02020");
   gdk_rgba_parse(&colors[2],"#f0a020");
   gdk_rgba_parse(&colors[3],"#f0d0a0");
   gdk_rgba_parse(&colors[4],"#f0d040");

   add_to_mainloop1(PRIORITY_DEFAULT, time_emeteor, do_emeteor, NULL);
   add_to_mainloop (PRIORITY_DEFAULT, 0.1,  do_meteor);
}

void meteor_ui()
{
   UIDO(NoMeteors       , );
   UIDO(MeteorFrequency , );
}

void meteor_draw(cairo_t *cr)
{
   P("meteor_draw %d %d\n",global.counter++,meteor.active);
   if (!meteor.active)
      return;
   drawcount++;
   double fraction = drawcount*time_draw_all/time_emeteor;
   P("meteor_draw %d %d %lf %lf\n",drawcount,meteor.active,time_emeteor/time_draw_all,fraction);
   cairo_save(cr);

   int c = meteor.colornum;
   cairo_set_line_width(cr,2);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
   cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
   cairo_move_to(cr,meteor.x1,meteor.y1);

   double xf = meteor.x1+fraction*(meteor.x2-meteor.x1);
   double yf = meteor.y1+fraction*(meteor.y2-meteor.y1);

   cairo_pattern_t *cp = cairo_pattern_create_linear(meteor.x1,yf,meteor.x2,yf);
   cairo_pattern_add_color_stop_rgba(cp, 0, colors[c].red,colors[c].green,colors[c].blue,0);
   cairo_pattern_add_color_stop_rgba(cp, 1, colors[c].red,colors[c].green,colors[c].blue,1);
   cairo_set_source(cr,cp);

   cairo_line_to(cr,xf,yf);
   cairo_stroke(cr);

   cairo_pattern_destroy(cp);

   cairo_restore(cr);
}

void meteor_erase()
{
   do_emeteor(NULL);
}

int do_emeteor(gpointer data)
{
   (void)data;
   P("do_emeteor %d\n",global.counter++);
   if (Flags.Done)
      return FALSE;
   if (!meteor.active || NOTACTIVE)
      return TRUE;
   if (!global.IsDouble)
   {
      int x = meteor.x1;
      int y = meteor.y1;
      int w = meteor.x2 - x;
      int h = meteor.y2 - y;
      if (w<0)
      {
	 x += w;
	 w = -w;
      }
      if (h<0)
      {
	 y += h;
	 h = -h;
      }
      x -= 1;
      y -= 1;
      w += 2;
      h += 2;
      myXClearArea(global.display,global.SnowWin,x,y,w,h,global.xxposures);
   }
   meteor.active = 0;
   drawcount = 0;
   return TRUE;
}

int do_meteor(void *d)
{
   (void)d;
   P("do_meteor %d\n",global.counter++);
   if (Flags.Done)
      return FALSE;

   if (!(NOTACTIVE || meteor.active || Flags.NoMeteors))
   {
      meteor.x1 = randint(global.SnowWinWidth);
      meteor.y1 = randint(global.SnowWinHeight/4);
      meteor.x2 = meteor.x1 + global.SnowWinWidth/10 - randint(global.SnowWinWidth/5);
      if (meteor.x2 == meteor.x1)
	 meteor.x2 +=5;
      meteor.y2 = meteor.y1 + global.SnowWinHeight/5 - randint(global.SnowWinHeight/5);
      if (meteor.y2 == meteor.y1)
	 meteor.y2 +=5;
      meteor.active   = 1;
      meteor.colornum = drand48()*NUMCOLORS;
   }

   if (Flags.MeteorFrequency < 0 || Flags.MeteorFrequency > 100)
      Flags.MeteorFrequency = DefaultFlags.MeteorFrequency;
   // when to start next meteor:
   float t = (0.5+drand48())*(Flags.MeteorFrequency*(0.1-time_meteor)/100 + time_meteor);
   P("do_meteor %f\n",t);
   add_to_mainloop (PRIORITY_DEFAULT, t, do_meteor);
   return FALSE;
}
