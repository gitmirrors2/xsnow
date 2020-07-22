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
#ifndef DOIT_H
#define DOIT_H
// calls macro's for elements of FLAGS
// DOIT_I is for x that should be output as 1234
// DOIT_S is for x that is a char*
// DOIT_L is for x that should be output as 0x123456

// these flags are not written to the config file and
// are no part of the ui
#define DOITALL \
   DOIT_I(Defaults)  \
   DOIT_I(Desktop) \
   DOIT_I(NoConfig)  \
   DOIT_I(NoMenu)  \
   DOIT_I(StopAfter)  \
   DOIT_I(Done)  \
   DOIT_I(WindNow)  \
   DOIT_I(XWinInfoHandling) \
   DOIT_L(WindowId)  \
   DOIT

// following flags are written to the config file and
// are part of the ui
#define DOIT  \
   DOIT_I(AllWorkspaces) \
   DOIT_I(BelowAll) \
   DOIT_I(BlowOffFactor)  \
   DOIT_I(CpuLoad)  \
   DOIT_I(DesiredNumberOfTrees)  \
   DOIT_I(Exposures)  \
   DOIT_I(FlakeCountMax)  \
   DOIT_I(FullScreen) \
   DOIT_I(KDEbg) \
   DOIT_I(MaxOnTrees)  \
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
   DOIT_I(NStars) \
   DOIT_I(OffsetS) \
   DOIT_I(OffsetW) \
   DOIT_I(OffsetX) \
   DOIT_I(OffsetY) \
   DOIT_I(Quiet) \
   DOIT_I(SantaSize) \
   DOIT_I(SantaSpeedFactor) \
   DOIT_I(SnowFlakesFactor) \
   DOIT_I(SnowSpeedFactor) \
   DOIT_I(TreeFill) \
   DOIT_I(UseAlpha)  \
   DOIT_I(UseBG) \
   DOIT_I(WhirlFactor) \
   DOIT_I(WindTimer) \
   \
   DOIT_S(BGColor) \
   DOIT_S(DisplayName) \
   DOIT_S(SnowColor) \
   DOIT_S(TreeColor) \
   DOIT_S(TreeType)   \
   \
   DOIT_I(Anarchy) \
   DOIT_I(Neighbours) \
   DOIT_I(Nbirds) \
   DOIT_I(ShowBirds) \
   DOIT_I(BirdsOnly) \
   DOIT_I(BirdsRestart)  \
   DOIT_I(ViewingDistance)  \
   DOIT_I(BirdsSpeed)  \
   DOIT_I(AttrFactor)  \
   DOIT_I(PrefDistance)


#endif
