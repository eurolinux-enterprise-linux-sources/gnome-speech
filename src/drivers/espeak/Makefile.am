serverinfodir = $(libdir)/bonobo/servers
serverinfo_in_files = GNOME_Speech_SynthesisDriver_Espeak.server.in
if BUILD_ESPEAK_DRIVER
bin_PROGRAMS = espeak-synthesis-driver
serverinfo_DATA = $(serverinfo_in_files:.server.in=.server)
endif

INCLUDES = -I$(top_srcdir)          \
	-I$(top_builddir)        \
	-I$(top_srcdir)/gnome-speech\
	$(gnome_speech_CFLAGS) \
	$(WARN_CFLAGS) \
	-I$(espeak_INCLUDES)

LDADD = $(top_builddir)/gnome-speech/libgnomespeech.la 	\
	$(gnome_speech_LIBS)	\
	$(espeak_LIBS)

espeak_synthesis_driver_SOURCES = \
	espeaksynthesisdriver.c \
	espeaksynthesisdriver.h \
	espeakspeaker.c \
	espeakspeaker.h

EXTRA_DIST = $(serverinfo_in_files)
CLEANFILES = $(serverinfo_DATA)
