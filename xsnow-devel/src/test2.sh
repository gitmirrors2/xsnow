#!/bin/sh
# -copyright-
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

XSNOW=xsnow
if [ -x ./xsnow ]; then
   XSNOW=./xsnow
fi
xvfb-run -s "-screen 0 1920x1080x24" sh -c "fvwm & sleep 4; $XSNOW -defaults -stopafter 3 >xsnow_out 2>&1"
if [ "$?" -ne 0 ] ; then
   echo "Problem in 'xvfb-run' command"
   cat xsnow_out
   exit 1
fi
grep -q "Halting because of flag -stopafter" xsnow_out
if [ "$?" -ne 0 ] ; then
   echo "xsnow did not end as expected"
   cat xsnow_out
   exit 1
fi
grep -q "no birds will fly" xsnow_out
if [ "$?" -ne 0 ] ; then
   echo "xsnow did not start as expected"
   cat xsnow_out
   exit 1
fi

exit 0

