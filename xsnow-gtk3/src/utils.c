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
#include <utils.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include "windows.h"

FILE *HomeOpen(const char *file,const char *mode, char **path)
{
   char *h = getenv("HOME");
   if (h == 0)
      return 0;
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
   //if (!GtkWinb)
   XClearArea(display, SnowWin, 0,0,0,0,True);
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

