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
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include "utils.h"
#include "windows.h"
#include "meteo.h"
#include "debug.h"
#include "version.h"


Pixel Black, White;

FILE *HomeOpen(const char *file,const char *mode, char **path)
{
   char *h = getenv("HOME");
   if (h == NULL)
      return NULL;
   char *home = strdup(h);
   (*path) = (char *) malloc(strlen(home)+strlen(file)+2);
   strcpy(*path,home);
   strcat(*path,"/");
   strcat(*path,file);
   FILE *f = fopen(*path,mode);
   free(home);
   return f;
}

void ClearScreen()
{
   // remove all our snow-related drawings
   XClearArea(display, SnowWin, 0,0,0,0,True);
   // Yes this is hairy: also remove meteorite.
   // It could be that a meteor region is still hanging around
   meteo_erase();
   XFlush(display);

}

float sq3(float x, float y, float z)
{
   return x*x + y*y + z*z;
}

float sq2(float x, float y)
{
   return x*x + y*y;
}

float fsignf(float x)
{
   if (x>0)
      return 1.0f;
   if (x<0)
      return -1.0f;
   return 0.0f;
}

Pixel AllocNamedColor(const char *colorName, Pixel dfltPix)
{
   XColor scrncolor;
   XColor exactcolor;
   if (XAllocNamedColor(display, DefaultColormap(display, screen),
	    colorName, &scrncolor, &exactcolor)) 
      return scrncolor.pixel;
   else
      return dfltPix;
}

Pixel IAllocNamedColor(const char *colorName, Pixel dfltPix)
{
   return AllocNamedColor(colorName, dfltPix) | 0xff000000;
}

int randint(int m)
{
   if (m <=0 )
      return 0;
   return drand48()*m;
}
// https://www.alanzucconi.com/2015/09/16/how-to-sample-from-a-gaussian-distribution/

double gaussian (double mean, double std, double min, double max) 
{
   double x;
   do {
      double v1, v2, s;
      do {
	 v1 = 2.0 * drand48() - 1.0;
	 v2 = 2.0 * drand48() - 1.0;
	 s = v1 * v1 + v2 * v2;
      } while (s >= 1.0 || s == 0);
      x = mean + v1 * sqrt((-2.0 * log(s)) / s) * std;
   } while (x < min || x > max);
   return x;
}

void sgaussian(long int seed)
{
   srand48(seed);
}

guint add_to_mainloop(gint prio,float time,GSourceFunc func,gpointer datap) 
{
   return g_timeout_add_full(prio,(int)1000*(time),(GSourceFunc)func,datap,NULL);
}

void remove_from_mainloop(guint *tag)
{
   if (*tag)
      g_source_remove(*tag);
   *tag = 0;
}

int is_little_endian(void)
{
   int endiantest = 1;
   return (*(char *)&endiantest) == 1;
}

void my_cairo_paint_with_alpha(cairo_t *cr, double alpha)
{
   if (alpha > 0.9)
      cairo_paint(cr);
   else
      cairo_paint_with_alpha(cr,alpha);
   P("%d alpha %f\n",counter++,alpha);
}

void PrintVersion()
{
   printf("Xsnow-%s\n%s\n"
	 , VERSION, VERSIONBY);
}

void rgba2color(GdkRGBA *c, char **s)
{
   *s = (char *)malloc(8);
   sprintf(*s,"#%02lx%02lx%02lx",lrint(c->red*255),lrint(c->green*255),lrint(c->blue*255));
}

