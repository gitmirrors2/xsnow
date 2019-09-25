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
#ifndef DOIT_H
#define DOIT_H
// calls macro's for elements of FLAGS
// DOIT_I is for x that should be output as 1234
// DOIT_S is for x that is a char*
// DOIT_L is for x that should be output as 0x123456

// these flags are not written to the config file and
// are no part of the ui
#define DOITALL \
   DOIT_I(defaults)  \
   DOIT_I(desktop) \
   DOIT_I(noconfig)  \
   DOIT_I(nomenu)  \
   DOIT_I(stopafter)  \
   DOIT_I(done)  \
   DOIT_I(windnow)  \
   DOIT_I(xwininfohandling) \
   DOIT_I(showstats) \
   DOIT_L(window_id)  \
   DOIT

// following flags are written to the config file and
// are part of the ui
#define DOIT  \
   DOIT_I(below) \
   DOIT_I(blowofffactor)  \
   DOIT_I(cpuload)  \
   DOIT_I(desired_number_of_trees)  \
   DOIT_I(exposures)  \
   DOIT_I(flakecountmax)  \
   DOIT_I(fullscreen) \
   DOIT_I(KDEbg) \
   DOIT_I(maxontrees)  \
   DOIT_I(MaxScrSnowDepth)  \
   DOIT_I(MaxWinSnowDepth) \
   DOIT_I(NoBlowSnow) \
   DOIT_I(NoFluffy) \
   DOIT_I(NoKeepSBot) \
   DOIT_I(NoKeepSnow) \
   DOIT_I(NoKeepSnowOnTrees) \
   DOIT_I(NoKeepSWin) \
   DOIT_I(NoMeteorites) \
   DOIT_I(NoRudolf) \
   DOIT_I(NoSanta) \
   DOIT_I(NoSnowFlakes) \
   DOIT_I(NoTrees) \
   DOIT_I(NoWind) \
   DOIT_I(nstars) \
   DOIT_I(offset_s) \
   DOIT_I(offset_w) \
   DOIT_I(offset_x) \
   DOIT_I(offset_y) \
   DOIT_I(quiet) \
   DOIT_I(SantaSize) \
   DOIT_I(SantaSpeedFactor) \
   DOIT_I(snowflakesfactor) \
   DOIT_I(SnowSpeedFactor) \
   DOIT_I(treefill) \
   DOIT_I(usealpha)  \
   DOIT_I(usebg) \
   DOIT_I(WhirlFactor) \
   DOIT_I(WindTimer) \
   DOIT_I(UseX11) \
   \
   DOIT_S(bgcolor) \
   DOIT_S(display_name) \
   DOIT_S(snowColor) \
   DOIT_S(trColor) \
   DOIT_S(TreeType)   

#endif
