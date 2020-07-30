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
SUBDIRS = Pixmaps

gamesdir = $(exec_prefix)/games
games_PROGRAMS = xsnow

xsnow_CPPFLAGS = $(XML_CFLAGS) $(GTK_CFLAGS) $(DBUS_CFLAGS) -DUSE_LIST_NODE_ALLOCATOR 
xsnow_LDADD    = $(XML_LIBS)   $(GTK_LIBS)   $(DBUS_LIBS)

xsnow_SOURCES = clocks.c ixpm.c main.c fallensnow.c wmctrl.c docs.c \
		clocks.h ixpm.h docs.h fallensnow.h pixmaps.h \
		pixmaps.c version.h wmctrl.h xsnow.h windows.h\
		flags.c flags.h \
		gaussian.c gaussian.h csvpos.c csvpos.h \
		dsimple.c dsimple.h clientwin.c clientwin.h \
		test1.sh test2.sh  xsnow.desktop \
		doit.h ui.c ui.h ui.xml \
		kdesetbg.c kdesetbg.h \
		transparent.c transparent.h debug.h \
		mainstub.cpp mainstub.h \
		birds.c kdtree.c birds.h kdtree.h globals.h doitb.h \
		hashtable.cpp hashtable.h

nodist_xsnow_SOURCES = ui_xml.h

BUILT_SOURCES = ui_xml.h

# create C code to get ui.xml in a string
# ISO C stipulates that the length of a string constant should
# not be larger than 4096, so we create a definition as in
# char xsnow_xml[] = {60,63,120,109,108,32,118,101,0};
#
ui_xml.h: ui.xml
	@echo "Creating $@ from $<"
	@echo "/* This file is generated from $< by Makefile */" > $@
	@echo "#pragma once" >> $@
	@echo "#define UI_XML_H" >> $@
	@echo "char xsnow_xml[] = {" >> $@
	@sed 's/^ *//' $< | awk -v FS="" \
	'BEGIN{for(n=0;n<256;n++)ord[sprintf("%c",n)]=n;}'\
	'{for (i=1;i<=NF;i++) printf "%d,", ord[$$i];'\
	'printf "%d,\n",ord["\n"];}' >> $@
	@echo "0};" >> $@

TESTS = test1.sh # test2.sh # test2 will fail if no X11 environment

desktopdir = $(datadir)/applications
desktop_DATA = xsnow.desktop

appicondir=$(datadir)/pixmaps
appicon_DATA=Pixmaps/xsnow.xpm

man6_MANS = xsnow.6
xsnow.6: xsnow
	./xsnow -manpage > $@

CLEANFILES = xsnow.6 ui_xml.h
