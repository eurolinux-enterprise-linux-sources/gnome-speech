/*
 * GNOME Speech - Speech services for the GNOME desktop
 *
 * Copyright 2007-2008 Sun Microsystems Inc.
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
 * espeakspeaker.c: Implements the EspeakSpeaker object--
 *                            a GNOME Speech driver for eSpeak
 *
 */

#include <unistd.h>
#include <string.h>
#include <libbonobo.h>
#include <glib/gmain.h>
#include <gnome-speech/gnome-speech.h>
#include <speak_lib.h>
#include "espeaksynthesisdriver.h"
#include "espeakspeaker.h"


static GObjectClass *parent_class;

static EspeakSpeaker *
espeak_speaker_from_servant (PortableServer_Servant *servant)
{
  return ESPEAK_SPEAKER(bonobo_object_from_servant (servant));
}



static void
espeak_add_parameter (EspeakSpeaker *espeak_speaker,
		      espeak_PARAMETER param,
		      const gchar *parameter_name,
		      gdouble min,
		      gdouble max,
		      parameter_set_func func)
{
  Speaker *speaker = SPEAKER(espeak_speaker);
  gdouble current;
  current = (gdouble)
    espeak_GetParameter(param, 1);


  speaker_add_parameter (speaker,
			 parameter_name,
			 min,
			 current,
			 max,
			 func);
}


#if 0
static void
espeak_add_insertindex (EspeakSpeaker *espeak_speaker,
			const gchar *parameter_name,
			gdouble min,
			gdouble max,
			parameter_set_func func)
{
  Speaker *speaker = SPEAKER(espeak_speaker);
  speaker_add_parameter (speaker,
			 parameter_name,
			 min,
			 -1, /* prevent reentrance during add */
			 max,
			 func);
}
#endif

static CORBA_boolean
espeak_registerSpeechCallback (PortableServer_Servant servant,
			       const GNOME_Speech_SpeechCallback callback,
			       CORBA_Environment *ev)
{
  EspeakSpeaker *s = espeak_speaker_from_servant (servant);
  Speaker *speaker = SPEAKER (s);
	
  /* Store reference to callback */

  speaker->cb = CORBA_Object_duplicate (callback, ev);

  return TRUE;
}



static CORBA_long
espeak_say (PortableServer_Servant servant,
	    const CORBA_char *text,
	    CORBA_Environment *ev)
{
  EspeakSpeaker *speaker = espeak_speaker_from_servant (servant);
  Speaker *s = SPEAKER (speaker);
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER(s->driver);

  return espeak_synthesis_driver_say (d, speaker, (gchar *) text);
}


static CORBA_boolean
espeak_stop (PortableServer_Servant servant,
	     CORBA_Environment *ev)
{
  EspeakSpeaker *speaker = espeak_speaker_from_servant (servant);
  Speaker *s = SPEAKER (speaker);
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER(s->driver);

  return espeak_synthesis_driver_stop (d);
}


static CORBA_boolean
espeak_isSpeaking (PortableServer_Servant servant,
		   CORBA_Environment *ev)
{
  EspeakSpeaker *speaker = espeak_speaker_from_servant (servant);
  Speaker *s = SPEAKER (speaker);
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER(s->driver);
  return espeak_synthesis_driver_is_speaking (d);
}


static void
espeak_wait (PortableServer_Servant servant,
	     CORBA_Environment *ev)
{
  EspeakSpeaker *speaker = espeak_speaker_from_servant (servant);
  Speaker *s = SPEAKER (speaker);
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER(s->driver);
  espeak_synthesis_driver_wait (d);
}


static void
espeak_speaker_init (EspeakSpeaker *speaker)
{
}



static void
espeak_speaker_finalize (GObject *obj)
{
  EspeakSpeaker *s = ESPEAK_SPEAKER (obj);
  Speaker *speaker = SPEAKER (s);

  if (speaker->driver)
    g_object_unref (speaker->driver);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}



static void
espeak_speaker_class_init (EspeakSpeaker *klass)
{
  SpeakerClass *class = SPEAKER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  
  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = espeak_speaker_finalize;

  /* Initialize parent class epv table */

  class->epv.say = espeak_say;
  class->epv.stop = espeak_stop;
  class->epv.wait = espeak_wait;
  class->epv.isSpeaking = espeak_isSpeaking;
  class->epv.registerSpeechCallback = espeak_registerSpeechCallback;
}




BONOBO_TYPE_FUNC (EspeakSpeaker,
		  speaker_get_type (),
		  espeak_speaker);



static gboolean
espeak_set_pitch (Speaker *speaker,
		  gdouble new_pitch)
{
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER (speaker->driver);

  espeak_synthesis_driver_set_param (d, espeakPITCH, (int) new_pitch);
  return TRUE;
}



static gboolean
espeak_set_pitch_fluctuation (Speaker *speaker,
			      gdouble new_pitch_fluctuation)
{
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER (speaker->driver);

  espeak_synthesis_driver_set_param (d, espeakRANGE, (int) new_pitch_fluctuation);
  return TRUE;
}



static gboolean
espeak_set_rate (Speaker *speaker,
		 gdouble new_rate)
{
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER (speaker->driver);

  espeak_synthesis_driver_set_param (d, espeakRATE, (int) new_rate);
  return TRUE;
}



static gboolean
espeak_set_volume (Speaker *speaker,
		   gdouble new_volume)
{
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER (speaker->driver);

  espeak_synthesis_driver_set_param (d, espeakVOLUME, (int) new_volume);
  return TRUE;
}


static gboolean
espeak_set_capitals (Speaker *speaker,
		     gdouble value)
{
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER (speaker->driver);

  espeak_synthesis_driver_set_param (d, espeakCAPITALS, (int) value);
  return TRUE;
}


EspeakSpeaker *
espeak_speaker_new (GObject *d,
		    const GNOME_Speech_VoiceInfo *voice_spec)
{
  EspeakSpeaker *speaker;
  Speaker *s;
  speaker = g_object_new (ESPEAK_SPEAKER_TYPE, NULL);
  s = SPEAKER(speaker);

  s->driver = g_object_ref (d);

  /* Set the specified voice */
  memset( &speaker->voice, 0, sizeof(espeak_VOICE));
  speaker->voice.name = g_strdup(voice_spec->name);
  speaker->voice.languages = NULL;
  speaker->voice.gender = (voice_spec->gender == GNOME_Speech_gender_male);
  espeak_SetVoiceByProperties(&speaker->voice);      

  /* See http://espeak.sourcearchive.com/documentation/1.39/src_2speak__lib_8h-source.html */

  espeak_add_parameter (speaker,
			espeakPITCH,
			"pitch",
			0,
			100,
			espeak_set_pitch);  
  espeak_add_parameter (speaker,
			espeakRANGE,
			"pitch fluctuation",
			0,
			100,
			espeak_set_pitch_fluctuation);
  espeak_add_parameter (speaker,
			espeakRATE,
			"rate",
			80,  /* src/setlengths.cpp in espeak */
			369, /* src/setlengths.cpp in espeak */
			espeak_set_rate);  
  espeak_add_parameter (speaker,
			espeakVOLUME,
			"volume",
			0,
			100,
			espeak_set_volume);  
  espeak_add_parameter (speaker,
			espeakCAPITALS,
			"capitals",
			0,
			3, 
			espeak_set_capitals);  

  return speaker;
}

