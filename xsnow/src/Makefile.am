# -copyright-
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
SUBDIRS = Pixmaps

gamesdir = $(exec_prefix)/games
games_PROGRAMS = xsnow

xsnow_CPPFLAGS = $(XML_CFLAGS) $(GTK_CFLAGS) $(X11_CFLAGS) 
xsnow_LDADD    = $(XML_LIBS)   $(GTK_LIBS)  $(X11_LIBS)

xsnow_SOURCES = clocks.c ixpm.c main.c fallensnow.c wmctrl.c docs.c \
		clocks.h ixpm.h docs.h fallensnow.h pixmaps.h \
		pixmaps.c version.h wmctrl.h xsnow.h windows.c windows.h\
		flags.c flags.h csvpos.c csvpos.h \
		dsimple.c dsimple.h clientwin.c clientwin.h \
		test1.sh xsnow.desktop \
		doit.h ui.c ui.h ui.xml \
		debug.h \
		mainstub.cpp mainstub.h \
		birds.c kdtree.c birds.h kdtree.h birdglobals.h doitb.h \
		hashtable.cpp hashtable.h \
		Santa.c Santa.h utils.c utils.h \
		scenery.c scenery.h snow.c snow.h \
		meteo.c meteo.h wind.c wind.h \
		stars.c stars.h blowoff.c blowoff.h \
		treesnow.c treesnow.h loadmeasure.c loadmeasure.h \
		snow_includes.h vroot.h \
		moon.c moon.h buttons.h undefall.inc \
		transwindow.c transwindow.h

nodist_xsnow_SOURCES = ui_xml.h snow_includes.h

BUILT_SOURCES = ui_xml.h snow_includes.h

EXTRA_DIST = gen_snow_includes.sh gen_ui_xml.sh

ui_xml.h: ui.xml
	@echo "Creating $@ from $<"
	$(top_srcdir)/src/gen_ui_xml.sh $(top_srcdir)

snow_includes.h: Pixmaps
	@echo "Creating $@"
	$(top_srcdir)/src/gen_snow_includes.sh $(top_srcdir)

TESTS = test1.sh

desktopdir = $(datadir)/applications
desktop_DATA = xsnow.desktop

appicondir=$(datadir)/pixmaps
appicon_DATA=Pixmaps/xsnow.svg

man6_MANS = xsnow.6
xsnow.6: xsnow
	./xsnow -manpage > $@

CLEANFILES = xsnow.6 ui_xml.h snow_includes.h .deps/* xsnow_out_2 xsnow_out_3  

