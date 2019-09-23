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
#include <stdio.h>
#include <stdlib.h>
#include "fallensnow.h"
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
static void drawquartcircle(int n, short int *y)  // nb: dimension of y > n+1
{
   int i;
   float n2 = n*n;
   for(i=0; i<=n; i++)
      y[i] = lrintf(sqrtf(n2 - i*i));
}

// insert a node at the start of the list 
void PushFallenSnow(FallenSnow **first, int window_id, int ws, int sticky,
      int x, int y, unsigned int w, unsigned int h) 
{
   FallenSnow *p = malloc(sizeof(FallenSnow));
   p->id         = window_id;
   p->map        = (unsigned char*) malloc(1);
   p->x          = x;
   p->y          = y;
   p->w          = w;
   p->h          = h;
   p->w8         = ((w-1)/8+1)*8;
   p->acth       = malloc(sizeof(*(p->acth))*w);
   p->desh       = malloc(sizeof(*(p->desh))*w);
   p->ws         = ws;
   p->sticky     = sticky;
   p->hidden     = 0;
   p->clean      = 0;
   int l = 0,i;
   for (i=0; i<w; i++)
   {
      p->acth[i] = 0; // specify l to get sawtooth effect
      p->desh[i] = h;
      l++;
      if (l > h)
	 l = 0;
   }

   if (w > h && window_id != 0)
   {
      drawquartcircle(h,&(p->desh[w-h-1]));
      for(i=0; i<=h; i++)
	 p->desh[i] = p->desh[w-1-i];
   }

   p->next  = *first;
   *first   = p;
}

// pop from list
int PopFallenSnow(FallenSnow **list)
{
   FallenSnow *next_node = NULL;

   if (*list == NULL) 
      return 0;

   next_node = (*list)->next;
   FreeFallenSnow(*list);
   *list = next_node;
   return 1;
}

// remove by id
int RemoveFallenSnow(FallenSnow **list, int id)
{
   if (*list == 0)
      return 0;

   FallenSnow *fallen = *list;
   if (fallen->id == id)
   {
      fallen = fallen->next;
      FreeFallenSnow(*list);
      *list = fallen;
      return 1;
   }

   FallenSnow *scratch = NULL;

   while (1)
   {
      if (fallen->next == NULL)
	 return 0;
      scratch = fallen->next;
      if (scratch->id == id)
	 break;
      fallen = fallen->next;
   }

   fallen->next = scratch->next;
   FreeFallenSnow(scratch);

   return 1;
}

void FreeFallenSnow(FallenSnow *fallen)
{
   free(fallen->map);
   free(fallen->acth);
   free(fallen->desh);
   //XDestroyRegion(fallen->region);
   free(fallen);
}

FallenSnow *FindFallen(FallenSnow *first, Window id)
{
   FallenSnow *fsnow = first;
   while(fsnow)
   {
      if(fsnow->id == id)
	 return fsnow;
      fsnow = fsnow->next;
   }
   return 0;
}

#ifndef USEX11
cairo_surface_t *CreateSurfaceFromFallen(FallenSnow *f, unsigned int snowcolor)
{
   const int format = CAIRO_FORMAT_ARGB32;
   int stride = cairo_format_stride_for_width(format,f->w);
   pixmap_from_fallen(f, stride, snowcolor);
   cairo_surface_t *s;
   s = cairo_image_surface_create_for_data (f->map, format, f->w, f->h, stride);
   return s;
}
#endif

#ifndef USEX11
// creates pixmap for format CAIRO_FORMAT_ARGB32
void pixmap_from_fallen(FallenSnow *f,int stride,unsigned int snowcolor)
{
   // stride in chars
   unsigned char *pixmap;
   pixmap = malloc(stride * f->h);
   memset(pixmap,0,stride*f->h);

   int intstride = stride/4;  // stride in ints
   int i;
   unsigned int color = snowcolor | 0xff000000;
   for (i=0; i<f->w; i++)
   {
      int j;
      for (j=f->acth[i]-1; j>=0; j--)
	 ((unsigned int*)pixmap)[i+intstride*(f->h-1-j)] = color;
   }
   free(f->map);
   f->map = pixmap;
}
#endif

#ifdef USEX11
void bitmap_from_fallen(FallenSnow *f,int stride)
{
   // stride in chars
   // todo: takes too much cpu
   int j;
   int p = 0;
   unsigned char *bitmap;
   bitmap = malloc(stride * f->h);

   for (j=0; j<f->h; j++)
   {
      int i;
      p = j*stride;
      for (i=0; i<f->w8; i+=8)
      {
	 int b = 0;
	 int m = 1;
	 int k;
	 int kmax = i+8;
	 if (kmax > f->w) kmax = f->w;
	 for (k=i; k<kmax; k++)
	 {
	    if(f->acth[k] >= f->h-j)
	       b |= m;
	    m <<= 1;
	 }
	 bitmap[p++] = b;
      }
   }
   free(f->map);
   f->map = bitmap;
}
#endif

// print list
void PrintFallenSnow(FallenSnow *list)
{
   FallenSnow *fallen = list;

   while (fallen != NULL) {
      int sumact = 0;
      int i;
      for(i=0; i<fallen->w; i++)
	 sumact += fallen->acth[i];
      printf("%#lx ws:%d x:%d y:%d w:%d c:%d s:%d\n", fallen->id, fallen->ws,
	    fallen->x, fallen->y, fallen->w, fallen->clean, sumact);
      fallen = fallen->next;
   }
}

