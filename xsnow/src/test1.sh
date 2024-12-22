#!/bin/sh
###### -copyright-
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

XSNOW=/usr/games/xsnow
if [ -x ./xsnow ]; then
   XSNOW=./xsnow
fi
# test if 'xsnow -h' more or less works:
$XSNOW -h | grep -q -i xsnow 
if [ $? -eq 0 ] ; then
   echo "PASSED: running $XSNOW -h"
else
   echo "FAILED: error in executing: $XSNOW -h"
   exit 1
fi
# test if all default values are substituted:
$XSNOW -h | grep -q DEFAULT_
if [ $? -eq 0 ] ; then
   echo "Not all default values are substituted:"
   $XSNOW -h | grep DEFAULT_
   exit 1
fi
