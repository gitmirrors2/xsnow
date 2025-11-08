/* 
 -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024 Willem Vermin
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
 * */
#pragma once

#define FLAGSFILE ".xsnowrc"
#define FLAKES_PER_SEC_PER_PIXEL 30
#define INITIALSCRPAINTSNOWDEPTH 8  /* Painted in advance */
#define INITIALYSPEED 120   // has to do with vertical flake speed
#define MAXBLOWOFFFACTOR 100
#define MAXSANTA	4    // santa types 0..4
#define MAXTANNENPLACES 10   // number of trees
#define MAXTREETYPE 8        // treetypes: 0..MAXTREETYPE Note that the last one is for extratree ;-)
#define MAXWSENS 0.4        // sensibility of flakes for wind
#define MAXXSTEP 2             /* drift speed max */
#define MAXYSTEP 10             /* falling speed max */
#define PIXINANIMATION	4    // nr of santa animations 
#define SANTASENS 0.2       // sensibility of Santa for wind
#define SANTASPEED0 12
#define SANTASPEED1 25
#define SANTASPEED2 50
#define SANTASPEED3 50
#define SANTASPEED4 70
//#define SNOWFLAKEMAXTYPE 13  // type is 0..SNOWFLAKEMAXTYPE
#define SNOWFREE 25  /* Must stay snowfree on display :) */
#define SNOWSPEED 0.7    // the higher, the speedier the snow
#define WHIRL 150
#define MAXVISWORKSPACES 100   // should be enough...
#define USE_EXTRATREE
#define ignore_atom                "IGNORE_ME"
