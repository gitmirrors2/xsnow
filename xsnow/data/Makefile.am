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
EXTRA_DIST = xsnow.appdata.xml.in xsnow.desktop.in
CLEANFILES = xsnow.appdata.xml xsnow.desktop

xsnow.appdata.xml: $(top_srcdir)/data/xsnow.appdata.xml.in
	sed "s/XSNOW_VERSION/$(PACKAGE_VERSION)/;s/XSNOW_DATE/$(DATE)/" $(top_srcdir)/data/xsnow.appdata.xml.in > $@

xsnow.desktop: $(top_srcdir)/data/xsnow.desktop.in
	sed "s/XSNOW_VERSION/$(PACKAGE_VERSION)/;s/XSNOW_DATE/$(DATE)/" $(top_srcdir)/data/xsnow.desktop.in > $@

appdatadir = $(datadir)/metainfo
appdata_DATA = xsnow.appdata.xml

desktopdir = $(datadir)/applications
desktop_DATA = xsnow.desktop
