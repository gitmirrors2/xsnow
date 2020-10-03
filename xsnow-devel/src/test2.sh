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
logfile=xsnow_out_2
>$logfile
# open Santa tab, click on train with Rudolph
xdo="xdotool mousemove --sync 200 50 click 1 mousemove --sync 470 320 click 1"
# testing without a compositing X window manager
xvfb-run -a -s "-screen 0 1920x1080x24" sh -c "$XSNOW -defaults -stopafter 5 >$logfile 2>&1& sleep 2; $xdo;sleep 8"
if [ "$?" -ne 0 ] ; then
   echo "Problem in 'xvfb-run' command" 1>&2
   cat $logfile 1>&2
   exit 1
fi
grep -q "Halting because of flag -stopafter" $logfile
if [ "$?" -ne 0 ] ; then
   echo "xsnow did not end as expected" 1>&2
   cat $logfile 1>&2
   exit 1
fi
grep -q "no birds will fly" $logfile
if [ "$?" -ne 0 ] ; then
   echo "xsnow did not start as expected" 1>&2
   cat $logfile 1>&2
   exit 1
fi
grep -q "Santa: 4 Rudolph: 1" $logfile
if [ "$?" -ne 0 ] ; then
   echo "xsnow did not react to mouse as expected" 1>&2
   cat $logfile 1>&2
   exit 77
fi
exit 0
