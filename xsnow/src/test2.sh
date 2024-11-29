#!/bin/bash
# -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021,2022,2023,2024 Willem Vermin
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
# run xsnow in virtual frame buffer (Xvfb)
# take a screenshot, and after a few seconds take another screenshot
# If these screenshots are identical, xsnow is not running and the test fails.
# Otherwise, xsnow is apparently running and the test succeeds.

if [ -z "$XSNOW_USESCREEN" ] ; then
# Use Xvfb as X server:
   XSNOW_USEXVFB=1 
# if not set, the tests will run on your screen: do not move your mouse.
fi

if [ "$XSNOW_FAST_CHECK" ] ; then
   echo "Skipping test2"
   exit 0
fi

apps="Xvfb xdotool scrot xdpyinfo"

missing=0
for app in $apps ; do
   if ! command -v "$app" ; then
      missing=`expr $missing + 1`
      echo "Not found: $app"
   fi
done

if [ "$missing" -ne 0 ] ; then
   echo "$0 FAILED because of missing program(s)"
   exit 1
fi


XSNOW=xsnow
if [ -x ./xsnow ]; then
   XSNOW=./xsnow
fi

rcfile="$HOME/.xsnowrc"
sumerr=0

killall -q -9 Xvfb

if [ "$XSNOW_USEXVFB" ] ; then
   export DISPLAY=":23.0"
   Xvfb "$DISPLAY" 2>&1 &
   p_xfvb="$!"
   # from https://unix.stackexchange.com/questions/310679/how-to-poll-for-xvfb-to-be-ready
   MAX_ATTEMPTS=300 # About 60 seconds
   COUNT=0
   echo -n "Waiting for Xvfb to be ready "
   while ! xdpyinfo -display "${DISPLAY}" >/dev/null 2>&1; do
     echo -n "."
     sleep 0.2
     COUNT=`expr "$COUNT" + 1`
     if [ "${COUNT}" -ge "${MAX_ATTEMPTS}" ]; then
       echo "  Gave up waiting for X server on ${DISPLAY}"
       exit 1
     fi
   done
   echo " Done - Xvfb is ready!"
else
   echo "Using your screen to run a test."
   echo "Do not move your mouse...."
   sleep 1
fi

first=`mktemp --tmpdir tmp.XXXXXXX.png`
second=`mktemp --tmpdir tmp.XXXXXXX.png`

killall -q -9 xsnow

"$XSNOW"  2>&1 &
p_xsnow="$!"

rcbak=`mktemp`
> "$rcbak"

# test if xsnow is changing screen

for i in `seq 20` ; do
   sleep 0.2
   test -f "$rcfile" && break
done

if [ ! -f "$rcfile" ] ; then
   echo "FAILED: cannot start xsnow"
   cp "$rcbak" "$rcfile"
   exit 1
fi

sleep 2  # wait to let xsnow paint it's things
scrot -z -o $first   # take first screenshot
sleep 3              # let xsnow run a few seconds
scrot -z -o $second  # and take second screenshot


cmp "$first" "$second"
rc="$?"
rm -f $first $second
if [ "$rc" != 0 ] ; then
   echo "PASSED: xsnow is running"
else
   echo "FAILED: xsnow is not running or changing the screen"
   sumerr=`expr "$sumerr" + 1`
   kill -9 "$p_xsnow"
   test "$XSNOW_USEXVFB" && kill -9 "$p_xfvb"
   cp "$rcbak" "$rcfile"
   exit 1
fi

# Test xsnow by pressing some buttons and look at the contents
# of ~/.xsnowrc.
# I tried to minimize the possibility of race conditions,
# but I am not sure if everything is ok.
# Therefore, the test succeeds as at most 3 tests fail.

waitfor()
{
   while test -f "$rcfile" ; do
      rm -f "$rcfile"
      sleep 0.2
   done
   kill -SIGUSR1 "$p_xsnow"
   local i
   for i in `seq 200` ; do
      #echo waitfor $1 $i
      sleep 0.1
      if test -f "$rcfile" ; then
	 if grep -q "$1" "$rcfile" ; then
	    return 0
	 fi
      fi
   done
   cat "$rcfile"
   return 1
}

click()
{
   waitfor "^id-$1 [0-9]"
   if [ "$?" != 0 ] ; then
      echo "NOT FOUND: id-$1"
      sumerr=`expr "$sumerr" + 1`
      return 1
   fi
   local coords=`cat $rcfile | awk "/^id-$1 /"' { print $2,$3; exit }'`
   if [ "$coords" ] ; then
      #echo click: $1 $coords
      xdotool mousemove $coords
      xdotool click 1
   else
      echo "NOT FOUND: coordinates of id-$1"
   fi
}

check()
{
   waitfor "^$1 [0-9]"
   if [ "$?" != 0 ] ; then
      echo "NOT FOUND: $1"
      sumerr=`expr "$sumerr" + 1`
      return 1
   fi
   local v=`cat $rcfile | awk "/^$1 /"' { print $2; exit }'`
   if test "$v" = "$2" ; then
      echo "PASSED $1 $2"
      return 0
   else
      echo "FAILED $1 $2, got: $v"
      sumerr=`expr "$sumerr" + 1`
      return 1
   fi
}

sleep 2

test -f "$rcfile" && cp "$rcfile" "$rcbak"
rm -f "$rcfile"

click alldefaults
check Aurora 1
click allvintage
check Aurora 0

click alldefaults
click snow
check NoSnowFlakes 0
click NoSnowFlakes
check NoSnowFlakes 1

click santa
check NoSanta 0
click NoSanta
check NoSanta 1

click scenery
check NoTrees 0
click NoTrees
check NoTrees 1

click celestials
check Stars 1
click Stars
check Stars 0

click birds
check ShowBirds 1
click ShowBirds
check ShowBirds 0

click settings
click general-default
check ThemeXsnow 1
click ThemeXsnow
check ThemeXsnow 0

kill -9 "$p_xsnow" 2>&1
cp "$rcbak" "$rcfile"

test "$XSNOW_USEXVFB" && kill -9 "$p_xfvb" 2>&1

if [ "$sumerr" = 0 ] ; then
   echo "All ok"
   exit 0
fi
if [ "$sumerr" -lt 4 ] ; then
   echo "Most ok, failures: $sumerr"
   exit 0
fi
echo "Not ok: number of failed tests: $sumerr"
exit 1
