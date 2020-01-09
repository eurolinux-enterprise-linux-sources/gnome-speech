serverinfodir = $(libdir)/bonobo/servers
if BUILD_SPEECH_DISPATCHER_DRIVER
bin_PROGRAMS = speechd-synthesis-driver
serverinfo_DATA = GNOME_Speech_SynthDriver_Speech_Dispatcher.server
endif

INCLUDES = -I$(top_srcdir)          \
	-I$(top_builddir)        \
	-I$(top_srcdir)/gnome-speech\
	$(gnome_speech_CFLAGS) \
	$(WARN_CFLAGS)

LDADD = $(top_builddir)/gnome-speech/libgnomespeech.la $(gnome_speech_LIBS)

speechd_synthesis_driver_SOURCES = \
	libspeechd.c \
	libspeechd.h \
	speechdsynthesisdriver.c \
	speechdsynthesisdriver.h \
	speechdspeaker.c \
	speechdspeaker.h

EXTRA_DIST = GNOME_Speech_SynthDriver_Speech_Dispatcher.server.in	\
             $(serverinfo_DATA)
CLEANFILES = $(serverinfo_DATA)
