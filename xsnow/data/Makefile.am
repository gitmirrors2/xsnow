EXTRA_DIST = xsnow.appdata.xml.in xsnow.desktop.in
CLEANFILES = xsnow.appdata.xml xsnow.desktop

xsnow.appdata.xml: xsnow.appdata.xml.in
	sed "s/XSNOW_VERSION/$(PACKAGE_VERSION)/;s/XSNOW_DATE/$(DATE)/" $< > $@

xsnow.desktop: xsnow.desktop.in
	sed "s/XSNOW_VERSION/$(PACKAGE_VERSION)/;s/XSNOW_DATE/$(DATE)/" $< > $@

appdatadir = $(datadir)/metainfo
appdata_DATA = xsnow.appdata.xml

desktopdir = $(datadir)/applications
desktop_DATA = xsnow.desktop
