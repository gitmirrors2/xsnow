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
#include <gtk/gtk.h>

extern void main_birds (GtkWidget *window);
extern void init_birds(int start);
extern float MaxViewingDistance(void);
//extern float PreferredViewingDistance(void);
extern float birds_get_range(void);
extern float birds_get_mean_dist(void);
extern void birds_set_attraction_point_relative(float x, float y, float z);
extern void birds_set_speed(int x);
extern void birds_set_scale(void);
extern void birds_init_color(const char *color);

