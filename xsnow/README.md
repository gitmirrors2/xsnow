## Do not modify this file, it is generated from README by bootstrap ##
  -copyright-
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
Xsnow: snow on your (Gnome)desktop.

Dependencies: 

   Includes:

    <assert.h>
    <dbus/dbus.h>
    <glib/gprintf.h>
    <glib.h>
    <gtk/gtk.h>
    <libxml/parser.h>
    <libxml/tree.h>
    <libxml/xpath.h>
    <math.h>
    <signal.h>
    <stdarg.h>
    <stdio.h>
    <stdlib.h>
    <string.h>
    <sys/time.h>
    <sys/times.h>
    <unistd.h>
    <X11/cursorfont.h>
    <X11/Intrinsic.h>
    <X11/Xatom.h>
    <X11/Xlib.h>
    <X11/Xos.h>
    <X11/xpm.h>
    <X11/Xutil.h>

Install:

./configure
make
sudo make install
# by default, will install xsnow in /usr/local/games/ and
# xsnow.6.gz in /usr/local/share/man/man6/

For developers: run './bootstrap' before './configure' if you changed
   Makefile.am, configure.ac, version.h and the like.
   Run 'make -distcheck' to create a tarball for distribution.

Copyright:
==========

you are permitted to copy, adapt and use the code in the program. 
HOWEVER: should you decide to alter the look and feel of the 
program, we expect that you change the name of the program from
'xsnow' to something else.

Disclaimer:
===========

There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

History:
========

Rick Jansen writes:

   In 1984 the first Macintosh program I wrote was a computer Christmas
   card, which showed a picture of a snowman and falling snow. Later
   a Father Xmas in his sleigh was added. I converted this program to
   an undying desk accessory in 1988 with several additions.
   But, little boys grow up, and when they are forced to a workstation
   they want their thingies there too. So here is Xsnow. For *you*.

    Happy winter and a merry X-mas!

    Rick Jansen

Willem Vermin writes:

   After years of fruitless waiting for a Gnome version of xsnow, I decided
   in December 2018 to try to make it myself, starting from Rick's xsnow. 
   I changed some algorithms, added and removed some flags and added some 
   visuals. If you want to get a feeling how the first version of xsnow 
   looked like, use the '-vintage' flag. 
   Xsnow now runs in Gnome and Fvwm, and in other 'simple' window 
   managers, I suppose. To run in in KDE: create a single-color background,
   (for example #123456) and use the color of this background: 
   xsnow -bg 0x123456 .

   Have fun, 
   Willem Vermin
