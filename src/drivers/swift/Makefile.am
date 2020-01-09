serverinfodir = $(libdir)/bonobo/servers
serverinfo_in_files = GNOME_Speech_SynthesisDriver_Swift.server.in
if BUILD_SWIFT_DRIVER
bin_PROGRAMS = swift-synthesis-driver
serverinfo_DATA = $(serverinfo_in_files:.server.in=.server)
endif

INCLUDES = -I$(top_srcdir)          \
	-I$(swift_INCLUDEDIR)       \
	-I$(top_builddir)           \
	-I$(top_srcdir)/gnome-speech\
	$(gnome_speech_CFLAGS)	    \
	$(WARN_CFLAGS)              \
	$(swift_INCLUDES)

LDADD = $(top_builddir)/gnome-speech/libgnomespeech.la $(gnome_speech_LIBS) $(swift_LIBS)

swift_synthesis_driver_SOURCES = \
	swiftsynthesisdriver.c \
	swiftsynthesisdriver.h \
	swiftspeaker.c \
	swiftspeaker.h

EXTRA_DIST = $(serverinfo_in_files)
CLEANFILES = $(serverinfo_DATA)
