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

extern int    Wind;
// Wind = 0: no wind
// Wind = 1: wind only affecting snow
// Wind = 2: wind affecting snow and santa
// Direction =  0: no wind direction I guess
// Direction =  1: wind from left to right
// Direction = -1: wind from right to left
extern int    Direction;
extern float  Whirl;
extern double WindTimer;
extern double WindTimerStart;
extern float  NewWind;
extern float  WindMax;


extern void wind_init(void);
extern int wind_ui(void);
