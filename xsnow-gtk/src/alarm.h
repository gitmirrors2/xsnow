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
#ifndef ALARM_H
#define ALARM_H
#define ALARMALL \
ALARM(blowoff,        0.50          )  /* time between blow snow off windows */ \
ALARM(clean,          1.00          )  /* time between cleaning desktop */ \
ALARM(emeteorite,     0.20          )  /* time between meteorites erasures */ \
ALARM(event,          0.01          )  /* time between checking events */ \
ALARM(fallen,         0.15 * factor )  /* time between redraw fallen snow */ \
ALARM(fuse,           1.00          )  /* time between testing on too much flakes */ \
ALARM(genflakes,      0.10          )  /* time between generation of flakes */ \
ALARM(meteorite,      3.10          )  /* time between meteorites */ \
ALARM(newwind,        1.00          )  /* time between changing wind */ \
ALARM(santa,          0.05 * factor )  /* time between drawings of santa */ \
ALARM(santa1,         0.01 * factor )  /* time between redrawings of santa */ \
ALARM(sfallen,        2.30          )  /* time between smoothing of fallen snow */ \
ALARM(snowflakes,     0.05 * factor )  /* time between redrawings of snow on trees */ \
ALARM(snow_on_trees,  0.50          )  /* time between redrawings of snow */ \
ALARM(stars,          0.50          )  /* time between drawing stars */ \
ALARM(testing,        0.10          )  /* time between testing code */ \
ALARM(tree,           0.05 * factor )  /* time between redrawings of trees */ \
ALARM(ui_check,       0.25          )  /* time between checking values from ui */ \
ALARM(ui_loop,        0.01          )  /* time between entering ui_loop */ \
ALARM(usanta,         0.05          )  /* time between update of santa position */ \
ALARM(ustars,         2.00          )  /* time between updating stars */ \
ALARM(wind,           0.10          )  /* time between starting or ending wind */ \
ALARM(wupdate,        0.50          )  /* time between getting windows information */ 
#endif
