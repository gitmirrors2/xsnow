######  -copyright-
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
SUBDIRS = Pixmaps


TOCC     = $(top_srcdir)/src/tocc.sh
TOASCII  = $(top_srcdir)/src/toascii.sh

gamesdir = $(exec_prefix)/games
games_PROGRAMS = xsnow

noinst_PROGRAMS = makeman

xsnow_CPPFLAGS    = $(GTK_CFLAGS) $(X11_CFLAGS) $(GSL_CFLAGS) -DLOCALEDIR=\"$(LOCALEDIR)\" \
		    -DLANGUAGES='"$(LANGUAGES)"'
xsnow_LDADD       = libxdo.a $(GTK_LIBS)   $(X11_LIBS)   $(GSL_LIBS) $(LIBINTL)
libxdo_a_CPPFLAGS = $(X11_CFLAGS)

if USE_NLS
LANGUAGES != for l in `sed 's/\#.*//' $(top_srcdir)/po/LINGUAS`; do echo $$l; done
xsnow_CPPFLAGS += -DENABLE_NLS=1
else
	LANGUAGES = 
endif

if MAKESELFREP
xsnow_CPPFLAGS += -DSELFREP
CFLAGS_FOR_BUILD += -DSELFREP
endif

noinst_LIBRARIES = libxdo.a
libxdo_a_SOURCES = xdo.c xdo_search.c xdo.h xdo_util.h xdo_version.h

xsnow_SOURCES = clocks.c ixpm.c main.c fallensnow.c wmctrl.c docs.c \
		clocks.h ixpm.h docs.h fallensnow.h pixmaps.h \
		pixmaps.c version.h wmctrl.h xsnow.h xsnow-constants.h windows.c windows.h\
		flags.c flags.h csvpos.c csvpos.h \
		dsimple.c dsimple.h clientwin.c clientwin.h \
		test1.sh test2.sh \
		doit.h ui.c ui.h ui.glade \
		debug.h \
		mainstub.cpp mainstub.h \
		birds.c kdtree.c birds.h kdtree.h birdglobals.h doitb.h \
		hashtable.cpp hashtable.h \
		Santa.c Santa.h utils.c utils.h \
		scenery.c scenery.h snow.c snow.h \
		meteor.c meteor.h wind.c wind.h \
		stars.c stars.h blowoff.c blowoff.h \
		treesnow.c treesnow.h loadmeasure.c loadmeasure.h \
		snow_includes.h vroot.h \
		moon.c moon.h buttons.h undefall.inc \
		transwindow.c transwindow.h \
		selfrep.c selfrep.h \
		aurora.c aurora.h \
		spline_interpol.c spline_interpol.h \
		safe_malloc.h mygettext.h gettext.h  \
		xpm2cairo-gdk.c xpm2cairo-gdk.h

nodist_xsnow_SOURCES = ui_xml.h snow_includes.h

makeman_SOURCES = makeman.c docs.c flags.c docs.h flags.h xsnow-constants.h \
		  version.h doit.h

# The man page and the help infor are created by xsnow.
# In case of cross compiling, xsnow cannot be used (it is made for another 
# architecture), therefore the program makeman is created.
# makeman is only capable to produce the man page. When cross compiling
# we need to make sure that the compiler and it's flags are taken from the
# system this Makefile is running on.
# The output of the compile step is removed: it generates positives for blhc
# which are of no importance, because makeman is used only during the creation
# of xsnow, and is discarded afterwards.


makeman$(EXEEXT): changelog.inc $(makeman_SOURCES)
	@echo "creating makeman";$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) -DVERSION=\"$(VERSION)\" -DMAKEMAN -I$(top_srcdir)/src -I. -o $@ \
		$(top_srcdir)/src/makeman.c $(top_srcdir)/src/docs.c $(top_srcdir)/src/flags.c  

if MAKESELFREP
tarfile_inc = tarfile.inc
else
tarfile_inc =
endif

BUILT_SOURCES = ui_xml.h snow_includes.h $(tarfile_inc)

tarfile = $(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz

$(tarfile):
	echo "Dummy tarfile." > $@

if MAKESELFREP
tarfile.inc: $(tarfile) $(TOASCII)
	@echo "Converting tarfile:"
	ls -l $(tarfile)
	env CC="$(CC_FOR_BUILD)" CFLAGS="$(CFLAGS_FOR_BUILD)" $(TOASCII) < $(tarfile) > $@ 
else
	tarfile.inc: 
	echo "No selfrep compiled in" | env "CC=$(CC_FOR_BUILD)" CFLAGS="$(CFLAGS_FOR_BUILD)" $(TOASCII) > $@
endif


EXTRA_DIST = gen_snow_includes.sh gen_ui_xml.sh tocc.sh toascii.sh xdo-copyright 

ui_xml.h: ui.glade $(top_srcdir)/src/gen_ui_xml.sh
	@echo "Creating $@ from ui.glade"
	$(top_srcdir)/src/gen_ui_xml.sh $(top_srcdir)

snow_includes.h: Pixmaps
	@echo "Creating $@"
	$(top_srcdir)/src/gen_snow_includes.sh $(top_srcdir)


TESTS = test1.sh test2.sh

changelog.inc: $(top_srcdir)/ChangeLog $(TOCC)
	$(TOCC) < $(top_srcdir)/ChangeLog > $@

xsnow-docs.$(OBJEXT): changelog.inc

xsnow-selfrep.$(OBJEXT): $(tarfile_inc)

appicondir=$(datadir)/pixmaps
appicon_DATA=Pixmaps/xsnow.svg

man6_MANS = xsnow.6
xsnow.6: makeman
	./makeman > $@

CLEANFILES = xsnow.6 ui_xml.h snow_includes.h xsnow_out_2 xsnow_out_3 \
	     changelog.inc tarfile.inc toascii toascii.c

