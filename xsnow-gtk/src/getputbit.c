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
#include "getputbit.h"
/*
 * a rude implementation of getting a bit from or setting a 
 * bit in an unsigned char*.
 *
 */
unsigned int getbit(unsigned char *a, unsigned int index)
{
   unsigned int charpos = index >>3;
   unsigned int bitpos  = index - (charpos<<3);
   return ((a[charpos]>>bitpos) & 1 );
}
void putbit(unsigned char *a, unsigned int index, unsigned int bit)
{
   unsigned int charpos = index >>3;
   unsigned int bitpos  = index - (charpos<<3);
   unsigned char m = 1<<bitpos;
   a[charpos] = (a[charpos] & ~m) | ((bit & 1)<<bitpos);
}
