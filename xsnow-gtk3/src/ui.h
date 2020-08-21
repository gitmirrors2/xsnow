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
#pragma once
extern void ui (int *argc, char **argv[]);
extern void ui_error_x11(int *argc, char **argv[]);
extern void ui_show_nflakes(int n);
extern void ui_set_birds_header(const char *text);
//extern void ui_set_vd_scale(void);
extern void ui_show_range_etc(void);
extern void ui_show_desktop_type(const char *s);
extern void ui_set_sticky(int x);
extern void ui_background(int m);

extern void ui_gray_ww(const int m);
extern void ui_gray_erase(const int m);
extern void ui_gray_below(const int m);
