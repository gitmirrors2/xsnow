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
/* name of button will be    Button.NStars 
 * glade:
 * id will be                stars-NStars
 * name of call back will be button_stars_NStars
 */
#define xsnow_snow       1
#define xsnow_santa      2
#define xsnow_scenery    3
#define xsnow_celestials 4
#define xsnow_birds      5
#define xsnow_settings   6

/*         code           type               name                 multiplier */
#define ALL_TOGGLES  \
   BUTTON(togglecode     ,xsnow_birds       ,BirdsOnly            ,1  )         \
   BUTTON(togglecode     ,xsnow_birds       ,FollowSanta          ,1  )         \
   BUTTON(togglecode     ,xsnow_birds       ,ShowAttrPoint        ,1  )         \
   BUTTON(togglecode     ,xsnow_birds       ,ShowBirds            ,1  )         \
   BUTTON(togglecode     ,xsnow_settings    ,AllWorkspaces        ,1  )         \
   BUTTON(togglecode     ,xsnow_settings    ,BelowAll             ,1  )         \
   BUTTON(togglecode     ,xsnow_settings    ,BelowConfirm         ,1  )         \
   BUTTON(togglecode     ,xsnow_settings    ,Exposures            ,1  )         \
   BUTTON(togglecode     ,xsnow_settings    ,FullScreen           ,1  )         \
   BUTTON(togglecode     ,xsnow_settings    ,UseBG                ,1  )         \
   BUTTON(togglecode     ,xsnow_celestials  ,NoMeteorites         ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_celestials  ,Halo                 ,1  )         \
   BUTTON(togglecode     ,xsnow_celestials  ,Moon                 ,1  )         \
   BUTTON(togglecode     ,xsnow_santa       ,NoSanta              ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_snow        ,BlowSnow             ,1  )         \
   BUTTON(togglecode     ,xsnow_snow        ,NoFluffy             ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_snow        ,NoKeepSBot           ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_snow        ,NoKeepSWin           ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_snow        ,NoKeepSnowOnTrees    ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_snow        ,NoSnowFlakes         ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_celestials  ,Stars                ,1  )         \
   BUTTON(togglecode     ,xsnow_scenery     ,NoTrees              ,-1 )   /*i*/ \
   BUTTON(togglecode     ,xsnow_celestials  ,NoWind               ,-1 )   /*i*/ \


#define ALL_RANGES \
   BUTTON(rangecode      ,xsnow_birds       ,Anarchy              ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,AttrFactor           ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,BirdsScale           ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,BirdsSpeed           ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,DisWeight            ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,FollowWeight         ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,Nbirds               ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,Neighbours           ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,PrefDistance         ,1  )         \
   BUTTON(rangecode      ,xsnow_birds       ,ViewingDistance      ,1  )         \
   BUTTON(rangecode      ,xsnow_settings    ,CpuLoad              ,1  )         \
   BUTTON(rangecode      ,xsnow_settings    ,OffsetS              ,-1 )   /*i*/ \
   BUTTON(rangecode      ,xsnow_settings    ,OffsetY              ,-1 )   /*i*/ \
   BUTTON(rangecode      ,xsnow_settings    ,Transparency         ,1  )         \
   BUTTON(rangecode      ,xsnow_celestials  ,HaloBright           ,1  )         \
   BUTTON(rangecode      ,xsnow_celestials  ,MoonSize             ,1  )         \
   BUTTON(rangecode      ,xsnow_celestials  ,MoonSpeed            ,1  )         \
   BUTTON(rangecode      ,xsnow_santa       ,SantaSpeedFactor     ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,BlowOffFactor        ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,FlakeCountMax        ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,MaxOnTrees           ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,MaxScrSnowDepth      ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,MaxWinSnowDepth      ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,SnowFlakesFactor     ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,SnowSize             ,1  )         \
   BUTTON(rangecode      ,xsnow_snow        ,SnowSpeedFactor      ,1  )         \
   BUTTON(rangecode      ,xsnow_celestials  ,NStars               ,1  )         \
   BUTTON(rangecode      ,xsnow_scenery     ,DesiredNumberOfTrees ,1  )         \
   BUTTON(rangecode      ,xsnow_scenery     ,TreeFill             ,1  )         \
   BUTTON(rangecode      ,xsnow_celestials  ,WhirlFactor          ,1  )         \
   BUTTON(rangecode      ,xsnow_celestials  ,WindTimer            ,1  )         \


#define ALL_COLORS \
   BUTTON(colorcode      ,xsnow_birds       ,BirdsColor           ,1  )         \
   BUTTON(colorcode      ,xsnow_settings    ,BGColor              ,1  )         \
   BUTTON(colorcode      ,xsnow_snow        ,SnowColor            ,1  )         \
   BUTTON(colorcode      ,xsnow_scenery     ,TreeColor            ,1  )         \


#define BUTTON(code, type, name, m) code(type,name,m)

#define ALL_BUTTONS \
   ALL_TOGGLES      \
   ALL_RANGES       \
   ALL_COLORS

