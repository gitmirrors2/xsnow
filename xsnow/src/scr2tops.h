/* 
   -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024,2025,2026 Willem Vermin
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
#pragma once
typedef struct _tops_t 
{
   int x;
   int y;
   int w;
}  tops_t;
#ifdef __cplusplus
extern "C" {
#endif
   extern void scr2tops(
	 int usescrot,    // whether to use scrot for screenshot or not
	 int kernelrows,  // number of rows to be matched
	 int kernelcols,  // number of cols to be matched
	 int max0,        // number of pixels that can be missed
	 int X,           // x-coordinate of area to consider
	 int Y,           // y-coordinate of area to consider
	 int Width,       // width of area to consider
	 int Height,      // height of area to consider
	 int min_y,       // tops with y < min_y are neglected
	 int min_width,   // minimal accepted width of top of window
	 int max_width,   // maximal accepted width of top of window
	 int removearea,  // remove shadowed tops if closer than removearea pixels from top above
			  // if < 0: do not remove shadowed tops
	 tops_t **tops,   // output
	 int *ntops
	 );
#ifdef __cplusplus
}
#endif
