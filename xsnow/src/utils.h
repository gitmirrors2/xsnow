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


#define SOMENUMBER 42
#define PRIORITY_DEFAULT   G_PRIORITY_LOW
#define PRIORITY_HIGH      G_PRIORITY_DEFAULT


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Intrinsic.h>
#include <gtk/gtk.h>
#include <math.h>

#ifdef HAVE_EXECINFO_H
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#define TRACEBACK_AVAILALBLE
#endif
#endif

extern guint   add_to_mainloop(gint prio,float time,GSourceFunc func);
extern guint   add_to_mainloop1(gint prio,float time,GSourceFunc func,gpointer datap);
extern void    remove_from_mainloop(guint *tag);
extern void    ClearScreen(void);
extern float   fsignf(float x);
extern FILE   *HomeOpen(const char *file,const char *mode,char **path);
extern float   sq2(float x, float y);
extern float   sq3(float x, float y, float z);
extern Pixel   IAllocNamedColor(const char *colorName, Pixel dfltPix);
extern Pixel   AllocNamedColor(const char *colorName, Pixel dfltPix);
extern int     randint(int m);
extern void    my_cairo_paint_with_alpha(cairo_t *cr, double alpha);
extern void    rgba2color(GdkRGBA *c, char **s);
extern void    Thanks(void);
extern void    myXClearArea(Display*display, Window win, int x, int y, int w, int h, int exposures);
extern int     ScaleChanged(int *prev);
#ifdef TRACEBACK_AVAILALBLE
extern void    traceback(void);
#endif

// obtain normally distributed number. The number will be between min and max:
extern double gaussian (double mean, double standard_deviation, double min, double max);
// seed the random generator (alternatively, srand48() can be used):
extern void sgaussian(long int seed);

extern int is_little_endian(void);
extern void PrintVersion(void);

