/*
 * GNOME Speech - Speech services for the GNOME desktop
 *
 * Copyright 2006 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * swiftspeaker.c: Implements the Swiftspeaker object--
 *                 a GNOME Speech driver for Cepstal's Swift TTS Engine
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libbonobo.h>
#include <glib/gmain.h>
#include <glib/gthread.h>
#include <gnome-speech/gnome-speech.h>
#include <swift.h>
#include "swiftspeaker.h"
#include "swiftsynthesisdriver.h"

static GObjectClass *parent_class;

static SwiftSpeaker *
swift_speaker_from_servant(PortableServer_Servant *servant)
{
	return (SWIFT_SPEAKER(bonobo_object_from_servant(servant)));
}

static CORBA_boolean
swift_speaker_registerSpeechCallback(
        PortableServer_Servant servant,
	const GNOME_Speech_SpeechCallback callback,
	CORBA_Environment *ev)
{
	SwiftSpeaker *s = swift_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);

	/* Store reference to callback */

	speaker->cb = CORBA_Object_duplicate(callback, ev);

	return TRUE;
}

static CORBA_long
swift_speaker_say(PortableServer_Servant servant,
		  const CORBA_char *text,
		  CORBA_Environment *ev)
{
	SwiftSpeaker *s = swift_speaker_from_servant (servant);
	Speaker *speaker = SPEAKER (s);
	SwiftSynthesisDriver *d = SWIFT_SYNTHESIS_DRIVER (speaker->driver);

	if (speaker_needs_parameter_refresh (speaker)) {
		speaker_refresh_parameters (speaker);
	}

	return swift_synthesis_driver_say (d, s, text);
}

static CORBA_boolean
swift_speaker_stop(PortableServer_Servant servant, CORBA_Environment *ev)
{
	SwiftSpeaker *s = swift_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);
	SwiftSynthesisDriver *d = SWIFT_SYNTHESIS_DRIVER (speaker->driver);
	return swift_synthesis_driver_stop (d);
}

static CORBA_boolean
swift_speaker_isSpeaking(PortableServer_Servant servant, CORBA_Environment *ev)
{
	SwiftSpeaker *s = swift_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);
	SwiftSynthesisDriver *d = SWIFT_SYNTHESIS_DRIVER (speaker->driver);
	return swift_synthesis_driver_is_speaking (d);
}

static void
swift_speaker_init(SwiftSpeaker *speaker)
{
}

static void
swift_speaker_finalize(GObject *obj)
{
	SwiftSpeaker *s = SWIFT_SPEAKER (obj);
	Speaker *speaker = SPEAKER (s);

	if (speaker->driver) {
		g_object_unref (speaker->driver);
	}

	if (parent_class->finalize) {
		parent_class->finalize (obj);
	}
}

static void
swift_speaker_class_init(SwiftSpeakerClass *klass)
{
	SpeakerClass *class = SPEAKER_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = swift_speaker_finalize;

	/* Initialize parent class epv table 
	 */
	class->epv.say = swift_speaker_say;
	class->epv.stop = swift_speaker_stop;
	class->epv.isSpeaking = swift_speaker_isSpeaking;
	class->epv.registerSpeechCallback = 
	        swift_speaker_registerSpeechCallback;
}

static gboolean
swift_speaker_set_rate(Speaker *speaker, gdouble new_rate)
{
	SwiftSpeaker *s = SWIFT_SPEAKER (speaker);
	swift_params_set_int(s->tts_params, "speech/rate", (int) new_rate);
	return TRUE;
}

static gboolean
swift_speaker_set_pitch(Speaker *speaker, gdouble new_pitch)
{
        /* [[[WDW - no clue really if this is a good way to do this 
	 * or not.]]] 
	 */
	SwiftSpeaker *s = SWIFT_SPEAKER (speaker);
	float pitch_shift = new_pitch / 100.0;
	swift_params_set_float(s->tts_params, 
			       "speech/pitch/shift", 
			       (float) pitch_shift);
	return TRUE;
}

static gboolean
swift_speaker_set_volume(Speaker *speaker, gdouble new_volume)
{
	SwiftSpeaker *s = SWIFT_SPEAKER (speaker);
	swift_params_set_int(s->tts_params, "audio/volume", (int) new_volume);
	return TRUE;
}

BONOBO_TYPE_FUNC(SwiftSpeaker, speaker_get_type(), swift_speaker);

SwiftSpeaker *
swift_speaker_new(const GObject *driver,
		  const GNOME_Speech_VoiceInfo *voice_spec)
{
        SwiftSynthesisDriver *d;
	SwiftSpeaker *speaker;
	Speaker *s;
	swift_voice *voice;

	speaker = g_object_new(SWIFT_SPEAKER_TYPE, NULL);
	s = SPEAKER (speaker);

	s->driver = g_object_ref (G_OBJECT(driver));
	speaker->tts_params = swift_params_new(NULL);

	/* Get a voice [[[WDW - probably could be a little more 
	 * efficient here and look for a criteria with "name=%s" 
	 * where %s = voice_spec->name.]]]
	 */
	d = SWIFT_SYNTHESIS_DRIVER (s->driver);
	voice = swift_port_find_first_voice(d->tts_port, NULL, NULL);

	while (voice) {
	        if (!strcmp(swift_voice_get_attribute(voice, "name"),
			    voice_spec->name)) {
	                speaker->tts_voice = voice;
	                break;
	        } else {
	                voice = swift_port_find_next_voice(d->tts_port);
	        }
	}

	/* Add parameters 
	 */
	speaker_add_parameter (s, "rate", 50, 170, 500,
			       swift_speaker_set_rate);
	speaker_add_parameter (s, "pitch", 50, 100, 250,
			       swift_speaker_set_pitch);
	speaker_add_parameter (s, "volume", 0, 100, 100,
			       swift_speaker_set_volume);

	return speaker;
}


