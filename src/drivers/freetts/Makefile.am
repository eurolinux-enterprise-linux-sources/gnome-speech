SUBDIRS=java native
if BUILD_FREETTS_DRIVER

freetts_driverdir = \
	$(prefix)/bin

freetts_driver_SCRIPTS = \
	freetts-synthesis-driver

endif
serverinfodir = \
	$(libdir)/bonobo/servers

EXTRA_DIST = GNOME_Speech_SynthesisDriver_FreeTTS.server.in GNOME_Speech_SynthesisDriver_FreeTTS.server user_addenda.txt

if BUILD_FREETTS_DRIVER
driverdatadir= $(pkgdatadir)/drivers/freetts
serverinfo_DATA = \
	GNOME_Speech_SynthesisDriver_FreeTTS.server
driverdata_DATA = user_addenda.txt
endif
