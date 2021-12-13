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
