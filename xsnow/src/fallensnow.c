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
   free(fallen->acth);
   free(fallen->desh);
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

