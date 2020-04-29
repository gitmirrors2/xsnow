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
#include <assert.h>
#include "snowflakes.h"
#include <X11/Xutil.h>

Snow *addFlake(Snow *flake)
{
   Snow *newflake = createFlake();
   newflake->prev = flake->prev;
   newflake->next = flake;
   flake->prev    = newflake;
   return newflake;
}

Snow *delFlake(Snow *flake)
{
   if (flake->prev)
      (flake->prev)->next = flake->next;
   Snow *retval = flake->next;
   if( flake->next)
      (flake->next)->prev = flake->prev;
   free(flake);
   return retval;
}

Snow *createFlake()
{
   Snow *flake = malloc(sizeof(Snow));
   flake->prev = 0;
   flake->next = 0;
   return flake;
}

int countSnow(Snow *flake, int print)
{
   int i=0;
   while(1)
   {
      ++i;
      if (print)
	 printf("printSnow: %d %d %d %d\n",i,flake->whatFlake,flake->w,flake->h);
      assert(flake->whatFlake <= 6);
      assert(flake->w>0);
      assert(flake->w<9);
      assert(flake->h>0);
      assert(flake->h<9);

      if (flake->next == NULL)
	 break;
      flake = flake->next;
   }
   return i;
}
