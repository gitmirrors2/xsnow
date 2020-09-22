#!/bin/bash
# -copyright-
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
for f in snow[0-9][0-9].xbm; do
   g=`basename $f .xbm`.xpm
   convert $f $g
   sed -i "1a \
      /* -xxx- \n */
	 1,/static char/{s/static char/XPM_TYPE/;s/\[/_xpm[/};1,/white/s/white/None/" $g 

   sed -i "2s/xxx/copyright/" $g
   grep -q copyright $f || sed -i "1i \
     /* -xxx- \n */" $f; sed -i "1s/xxx/copyright/" $f
   
   echo "$f -> $g"
done
