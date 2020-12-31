/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020 Willem Vermin
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
/*         code           type         name                 multiplier */
#define ALL_TOGGLES  \
   BUTTON(togglecode     ,birds       ,BirdsOnly            ,1  )         \
   BUTTON(togglecode     ,birds       ,FollowSanta          ,1  )         \
   BUTTON(togglecode     ,birds       ,ShowAttrPoint        ,1  )         \
   BUTTON(togglecode     ,birds       ,ShowBirds            ,1  )         \
   BUTTON(togglecode     ,general     ,AllWorkspaces        ,1  )         \
   BUTTON(togglecode     ,general     ,BelowAll             ,1  )         \
   BUTTON(togglecode     ,general     ,BelowConfirm         ,1  )         \
   BUTTON(togglecode     ,general     ,Exposures            ,1  )         \
   BUTTON(togglecode     ,general     ,FullScreen           ,1  )         \
   BUTTON(togglecode     ,general     ,UseBG                ,1  )         \
   BUTTON(togglecode     ,meteo       ,NoMeteorites         ,-1 )   /*i*/ \
   BUTTON(togglecode     ,moon        ,Halo                 ,1  )         \
   BUTTON(togglecode     ,moon        ,Moon                 ,1  )         \
   BUTTON(togglecode     ,santa       ,NoSanta              ,-1 )   /*i*/ \
   BUTTON(togglecode     ,snow        ,BlowSnow             ,1  )         \
   BUTTON(togglecode     ,snow        ,NoFluffy             ,-1 )   /*i*/ \
   BUTTON(togglecode     ,snow        ,NoKeepSBot           ,-1 )   /*i*/ \
   BUTTON(togglecode     ,snow        ,NoKeepSWin           ,-1 )   /*i*/ \
   BUTTON(togglecode     ,snow        ,NoKeepSnowOnTrees    ,-1 )   /*i*/ \
   BUTTON(togglecode     ,snow        ,NoSnowFlakes         ,-1 )   /*i*/ \
   BUTTON(togglecode     ,stars       ,Stars                ,1  )         \
   BUTTON(togglecode     ,tree        ,NoTrees              ,-1 )   /*i*/ \
   BUTTON(togglecode     ,wind        ,NoWind               ,-1 )   /*i*/ \


#define ALL_RANGES \
   BUTTON(rangecode      ,birds       ,Anarchy              ,1  )         \
   BUTTON(rangecode      ,birds       ,AttrFactor           ,1  )         \
   BUTTON(rangecode      ,birds       ,BirdsScale           ,1  )         \
   BUTTON(rangecode      ,birds       ,BirdsSpeed           ,1  )         \
   BUTTON(rangecode      ,birds       ,DisWeight            ,1  )         \
   BUTTON(rangecode      ,birds       ,FollowWeight         ,1  )         \
   BUTTON(rangecode      ,birds       ,Nbirds               ,1  )         \
   BUTTON(rangecode      ,birds       ,Neighbours           ,1  )         \
   BUTTON(rangecode      ,birds       ,PrefDistance         ,1  )         \
   BUTTON(rangecode      ,birds       ,ViewingDistance      ,1  )         \
   BUTTON(rangecode      ,general     ,CpuLoad              ,1  )         \
   BUTTON(rangecode      ,general     ,OffsetS              ,-1 )   /*i*/ \
   BUTTON(rangecode      ,general     ,OffsetY              ,-1 )   /*i*/ \
   BUTTON(rangecode      ,general     ,Transparency         ,1  )         \
   BUTTON(rangecode      ,moon        ,HaloBright           ,1  )         \
   BUTTON(rangecode      ,moon        ,MoonSize             ,1  )         \
   BUTTON(rangecode      ,moon        ,MoonSpeed            ,1  )         \
   BUTTON(rangecode      ,santa       ,SantaSpeedFactor     ,1  )         \
   BUTTON(rangecode      ,snow        ,BlowOffFactor        ,1  )         \
   BUTTON(rangecode      ,snow        ,FlakeCountMax        ,1  )         \
   BUTTON(rangecode      ,snow        ,MaxOnTrees           ,1  )         \
   BUTTON(rangecode      ,snow        ,MaxScrSnowDepth      ,1  )         \
   BUTTON(rangecode      ,snow        ,MaxWinSnowDepth      ,1  )         \
   BUTTON(rangecode      ,snow        ,SnowFlakesFactor     ,1  )         \
   BUTTON(rangecode      ,snow        ,SnowSize             ,1  )         \
   BUTTON(rangecode      ,snow        ,SnowSpeedFactor      ,1  )         \
   BUTTON(rangecode      ,stars       ,NStars               ,1  )         \
   BUTTON(rangecode      ,tree        ,DesiredNumberOfTrees ,1  )         \
   BUTTON(rangecode      ,tree        ,TreeFill             ,1  )         \
   BUTTON(rangecode      ,wind        ,WhirlFactor          ,1  )         \
   BUTTON(rangecode      ,wind        ,WindTimer            ,1  )         \


#define ALL_COLORS \
   BUTTON(colorcode      ,birds       ,BirdsColor           ,1  )         \
   BUTTON(colorcode      ,general     ,BGColor              ,1  )         \
   BUTTON(colorcode      ,snow        ,SnowColor            ,1  )         \
   BUTTON(colorcode      ,tree        ,TreeColor            ,1  )         \


#define BUTTON(code, type, name, m) code(type,name,m)

#define ALL_BUTTONS \
   ALL_TOGGLES      \
   ALL_RANGES       \
   ALL_COLORS

