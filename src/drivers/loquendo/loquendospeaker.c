/*
 * GNOME Speech - Speech services for the GNOME desktop
 *
 * Copyright 2003 Sun Microsystems Inc.
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
 * loquendospeaker.c: Implements the LoquendoSpeaker object--
 *                 a GNOME Speech driver for Loquendo TTS SDK
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libbonobo.h>
#include <glib/gmain.h>
#include <glib/gthread.h>
#include <gnome-speech/gnome-speech.h>
#include <loqtts.h>
#include "loquendospeaker.h"
#include "loquendosynthesisdriver.h"


static GObjectClass *parent_class;


static LoquendoSpeaker * loquendo_speaker_from_servant(PortableServer_Servant *servant)
{
	return(LOQUENDO_SPEAKER(bonobo_object_from_servant(servant)));
}



static CORBA_boolean loquendo_registerSpeechCallback(PortableServer_Servant servant,
											         const GNOME_Speech_SpeechCallback callback,
													 CORBA_Environment *ev)
{
	LoquendoSpeaker *s = loquendo_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);

	/* Store reference to callback */

	speaker->cb = CORBA_Object_duplicate(callback, ev);

	return TRUE;
}


static CORBA_long loquendo_say(PortableServer_Servant servant,
							   const CORBA_char *text,
							   CORBA_Environment *ev)
{
	LoquendoSpeaker *s = loquendo_speaker_from_servant (servant);
	Speaker *speaker = SPEAKER (s);

	return loquendo_synthesis_driver_say (LOQUENDO_SYNTHESIS_DRIVER(speaker->driver),
				                	      s, (gchar *) text);
} 


static CORBA_boolean loquendo_stop(PortableServer_Servant servant, CORBA_Environment *ev)
{
	LoquendoSpeaker *s = loquendo_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);
	speaker->parameter_refresh = TRUE;
	return loquendo_synthesis_driver_stop (LOQUENDO_SYNTHESIS_DRIVER(speaker->driver));
}


static void loquendo_wait(PortableServer_Servant servant, CORBA_Environment *ev)
{
	LoquendoSpeaker *s = loquendo_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);
	loquendo_synthesis_driver_wait (LOQUENDO_SYNTHESIS_DRIVER(speaker->driver));
}


static CORBA_boolean loquendo_isSpeaking(PortableServer_Servant servant, CORBA_Environment *ev)
{
	LoquendoSpeaker *s = loquendo_speaker_from_servant(servant);
	Speaker *speaker = SPEAKER (s);
	speaker->parameter_refresh = TRUE;
	return loquendo_synthesis_driver_is_speaking (LOQUENDO_SYNTHESIS_DRIVER(speaker->driver));
}


static void loquendo_speaker_init(LoquendoSpeaker *speaker)
{
}


static void loquendo_speaker_finalize(GObject *obj)
{
	LoquendoSpeaker *s = LOQUENDO_SPEAKER (obj);
	Speaker *speaker = SPEAKER (s);

	if (speaker->driver)
		g_object_unref (speaker->driver);
	if (parent_class->finalize)
      parent_class->finalize (obj);
}


static void loquendo_speaker_class_init(LoquendoSpeaker *klass)
{
	SpeakerClass *class = SPEAKER_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
  
	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = loquendo_speaker_finalize;

	/* Initialize parent class epv table */

	class->epv.say = loquendo_say;
	class->epv.stop = loquendo_stop;
	class->epv.wait = loquendo_wait;
	class->epv.isSpeaking = loquendo_isSpeaking;
	class->epv.registerSpeechCallback = loquendo_registerSpeechCallback;
}


BONOBO_TYPE_FUNC(LoquendoSpeaker, speaker_get_type(), loquendo_speaker);



static gboolean loquendo_set_pitch (Speaker *speaker, gdouble new_pitch)
{
   ttsResultType r;
   LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER (speaker->driver);
   r=ttsSetPitch(d->handle,(int) new_pitch);
   return (tts_OK==r);
}


static gboolean loquendo_set_speed (Speaker *speaker, gdouble new_speed)
{
   ttsResultType r;
   LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER (speaker->driver);

   r=ttsSetSpeed(d->handle,(int) new_speed);
   return (tts_OK==r);
}


static gboolean loquendo_set_volume (Speaker *speaker, gdouble new_volume)
{
   ttsResultType r;
   LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER (speaker->driver);
	r=ttsSetVolume(d->handle,(int) new_volume);

   return (tts_OK==r);
}


LoquendoSpeaker *loquendo_speaker_new(const GObject *driver,
									  const GNOME_Speech_VoiceInfo *voice_spec)
{
	LoquendoSpeaker *speaker;
	Speaker *s;
	unsigned int minimo, normal, maximo;


	ttsHandleType hVoice;
	ttsResultType err;
	LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER (driver);

	speaker = g_object_new(LOQUENDO_SPEAKER_TYPE, NULL);
	s = SPEAKER (speaker);

	s->driver = g_object_ref (G_OBJECT(driver));

	if ( (ttsTestVoice(NULL, voice_spec->name, 16000, "l")) == ttsTRUE)
	{

 	   err = ttsNewVoice(&hVoice, d->handle, voice_spec->name, 16000, "l");
      if (err != tts_OK)
	  	{	
			return NULL;
  	   }

		err = ttsActivateVoice(hVoice);
		if (tts_OK != err) 
		 {
   	    return NULL;
   	 }
	}

	ttsGetPitchRange(d->handle, &minimo, &normal, &maximo);
	speaker_add_parameter(s, "pitch", minimo, normal, maximo, loquendo_set_pitch);  

	ttsGetSpeedRange(d->handle, &minimo, &normal, &maximo);
	speaker_add_parameter(s, "rate", minimo, normal, maximo, loquendo_set_speed);  

	ttsGetVolumeRange(d->handle, &minimo, &normal, &maximo);
	speaker_add_parameter(s, "volume", minimo, normal, maximo, loquendo_set_volume);  


	return speaker;

}

