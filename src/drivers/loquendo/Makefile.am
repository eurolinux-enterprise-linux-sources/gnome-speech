serverinfodir = $(libdir)/bonobo/servers
serverinfo_in_files = GNOME_Speech_SynthesisDriver_Loquendo.server.in
if BUILD_LOQUENDO_DRIVER
bin_PROGRAMS = loquendo-synthesis-driver
serverinfo_DATA = $(serverinfo_in_files:.server.in=.server)
endif

INCLUDES = -I$(top_srcdir)          \
	-I$(top_builddir)        \
	-I$(top_srcdir)/gnome-speech\
	$(gnome_speech_CFLAGS) \
	$(WARN_CFLAGS) \
	$(loquendo_INCLUDES)

LDADD = $(top_builddir)/gnome-speech/libgnomespeech.la 	\
	$(gnome_speech_LIBS)	\
	$(loquendo_LIBS)

loquendo_synthesis_driver_SOURCES = \
	loquendosynthesisdriver.c \
	loquendosynthesisdriver.h \
	loquendospeaker.c \
	loquendospeaker.h

EXTRA_DIST = GNOME_Speech_SynthesisDriver_Loquendo.server.in GNOME_Speech_SynthesisDriver_Loquendo.server 
CLEANFILES = $(serverinfo_DATA)
