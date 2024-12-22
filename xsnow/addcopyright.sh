#!/bin/sh
# -copyright-
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
crfile=`mktemp`

cat <<eof > $crfile
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
eof

# strange initializations: to prevent recognizing them as
# _copyright_ and end_copyright_ (replace _ with -)
txt1="-""copyright-"
txt2="-end""copyright-"

n=0

while [ "$1" ] ; do
   f="$1"
   case "$f" in
      *.git*|*.svn*|*.deps*)
	 shift
	 continue
	 ;;
   esac
   n1=`grep -n -m1 -- "$txt1" "$f" | sed 's/:.*//'` 
   if [ -z "$n1" ] ; then 
      echo "$f: no $txt1"
      shift
      continue
   fi
   n2=`grep -n -m1 -- "$txt2" "$f" | sed 's/:.*//'` 
   if [ -z "$n2" ] ; then 
      echo "$f: no $txt2"
      shift
      continue
   fi

   if [ $n2 -le $n1 ] ; then  
      echo "$f: $n2:$txt2 before $n1:$txt1"
      shift
      continue
   fi

   if [ `grep -c -- "$txt1" "$f"` != 1 ] ; then
      echo "$f: more than one $txt1"
      shift
      continue
   fi

   if [ `grep -c -- "$txt2" "$f"` != 1 ] ; then
      echo "$f: more than one $txt2"
      shift
      continue
   fi

   sed "/$txt1/,/$txt2/{//!d}" "$f" > tmpfile
   if [ "$?" -eq 0 ]; then
      sed "/$txt1/r $crfile" tmpfile > tmpfile1
      if [ "$?" -eq 0 ]; then
	 cp tmpfile1 "$f"
	 n=`expr $n + 1`
      fi
   fi

   shift
done
rm -f tmpfile tmpfile1
echo "$n files copyrighted"

