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
 * espeaksynthesisdriver.c: Implements the EspeakSynthesisDriver object--
 *                            a GNOME Speech driver for eSpeak
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libbonobo.h>
#include <glib.h>
#include <glib/gmain.h>
#include <speak_lib.h>
#include <gnome-speech/gnome-speech.h>
#include "espeaksynthesisdriver.h"
#include "espeakspeaker.h"
 
#define VERSION_LENGTH 20

static gint utterance_id = 0;

typedef struct {
  GNOME_Speech_SpeechCallback cb;
  gboolean speech_is_started;
  gint id;
} t_user_data;

typedef struct {
  espeak_VOICE *voice;
  char *text;
  t_user_data *user_data;
} t_utterance;

static GObjectClass *parent_class;

static EspeakSynthesisDriver *
espeak_synthesis_driver_from_servant (PortableServer_Servant *servant)
{
  return ESPEAK_SYNTHESIS_DRIVER(bonobo_object_from_servant (servant));
}


static int
espeak_synthesis_driver_index_callback (short* wav,
					int numsamples,
					espeak_EVENT *event)
{
  if (event && event->user_data)
    {
      t_user_data *user_data = event->user_data;
      GNOME_Speech_speech_callback_type type = 
        GNOME_Speech_speech_callback_speech_started;
      
      gboolean a_callback_is_called = TRUE;

      switch(event->type)
	{
	case espeakEVENT_SENTENCE:
	  type = GNOME_Speech_speech_callback_speech_started;
	  user_data->speech_is_started = TRUE;
	  break;

	case espeakEVENT_WORD:
	case espeakEVENT_MARK:
	case espeakEVENT_END:
	  type = GNOME_Speech_speech_callback_speech_progress;
	  break;

	case espeakEVENT_MSG_TERMINATED:
	  if (user_data->speech_is_started)
	    {
	      type = GNOME_Speech_speech_callback_speech_ended;
	    }	  
	  break;

	case espeakEVENT_PLAY:
	default:
	  a_callback_is_called = FALSE;
	  break;
	}

      if (a_callback_is_called)
	{
	  CORBA_Environment ev;
	  CORBA_exception_init (&ev);
	  GNOME_Speech_SpeechCallback_notify (user_data->cb,
					      type,
					      user_data->id,
					      event->text_position,
					      &ev);

	  if (event->type == espeakEVENT_MSG_TERMINATED)
	    {
	      CORBA_Object_release (user_data->cb, &ev);
	      g_free(user_data);
	    }

	  CORBA_exception_free (&ev);
	}
    }

  return 0;
}



static CORBA_string
espeak__get_driverName (PortableServer_Servant servant,
			CORBA_Environment * ev)
{
  return CORBA_string_dup ("eSpeak GNOME Speech Driver");
}




static CORBA_string
espeak__get_synthesizerName (PortableServer_Servant servant,
			     CORBA_Environment * ev)
{
  return CORBA_string_dup ("eSpeak");
}



static CORBA_string
espeak__get_driverVersion (PortableServer_Servant aservant,
			   CORBA_Environment * ev)
{
  return CORBA_string_dup ("0.3");
}



static CORBA_string
espeak__get_synthesizerVersion (PortableServer_Servant
				servant,
				CORBA_Environment * ev)
{
  return CORBA_string_dup (espeak_Info(NULL));
}


static CORBA_boolean
espeak_driverInit (PortableServer_Servant servant,
		   CORBA_Environment *ev)
{
  EspeakSynthesisDriver *d = espeak_synthesis_driver_from_servant (servant);

  if (!d->initialized) {

    espeak_SetSynthCallback( espeak_synthesis_driver_index_callback);

#if ESPEAK_API_REVISION>=2
    espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 200, NULL, 0);
#else
    espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 200, NULL);
#endif

    d->text_idle = 0;
    d->initialized = TRUE;
  }
  return d->initialized;
}


static CORBA_boolean
espeak_isInitialized (PortableServer_Servant servant,
		      CORBA_Environment *ev)
{
  EspeakSynthesisDriver *driver = espeak_synthesis_driver_from_servant (servant);
  return driver->initialized;
}

/* TBD: check how to integrate this code */
/* static GSList * */
/* get_voice_list_with_spec(GSList *l, */
/*                          const GNOME_Speech_VoiceInfo *info) */
/* { */
/*     GSList *tmp, *new; */
/*     GNOME_Speech_VoiceInfo *i, *new_info; */

/*     fprintf(stderr, "get_voice_list_with_spec called. [DONE].\n"); */
/*     new = NULL; */
/*     tmp = l; */
/*     while (tmp) { */
/*         i = (GNOME_Speech_VoiceInfo *) tmp->data; */
/*         if ((strlen(info->name) ? !strcmp (i->name, info->name) : 1) &&  */
/*             (strlen(info->language) ? !strcmp(i->language, info->language)  */
/*                                     : 1)) { */
/*             new_info = GNOME_Speech_VoiceInfo__alloc(); */
/*             new_info->language = CORBA_string_dup(info->language); */
/*             new_info->name = CORBA_string_dup(info->name); */
/*             new_info->gender = info->gender; */
/*             new = g_slist_prepend(new, new_info); */
/*         } */
/*         tmp = tmp->next; */
/*     } */
/*     new = g_slist_reverse(new); */

/*     return(new); */
/* } */


static GSList *
get_voice_list(void)
{
    const espeak_VOICE **voices;
    GSList *l = NULL;
    GSList *n_lang = NULL;
    GSList *n_lang_en = NULL;
    GSList *n_tail = NULL;
    int i=0;

    char* a_language = getenv("LANGUAGE");
    if ((a_language == NULL) 
	|| strlen(a_language) < 2) {
        a_language = getenv("LANG");
	if ((a_language == NULL) 
	    || strlen(a_language) < 2) {
	  a_language="en";
      }
    }

    voices = espeak_ListVoices(NULL);
    
    for (i = 0; voices[i] != NULL; i++) {
        GNOME_Speech_VoiceInfo *info;

        info = GNOME_Speech_VoiceInfo__alloc();
	info->language = CORBA_string_dup(1+(voices[i]->languages));
        info->name = CORBA_string_dup(voices[i]->name);
        if (voices[i]->gender == 0 || voices[i]->gender == 1) {
            info->gender = GNOME_Speech_gender_male;
        } else if (voices[i]->gender == 2) {
            info->gender = GNOME_Speech_gender_female;
        }
	
        l = g_slist_prepend(l, info);

	if (n_tail == NULL) {
	    n_tail = l;
	  }

	if ((NULL == n_lang)
	    && info->language
	    && (0 == strncmp(info->language, a_language, 2))) {
	    n_lang = l;
	  }

	if ((NULL == n_lang_en)
	    && info->language
	    && (0 == strncmp(info->language, "en", 2))) {
	    n_lang_en = l;
	  }
    }

    // last element = current language
    if (n_lang == NULL) {
	n_lang = n_lang_en; // default = english
      }

    if (n_lang && n_lang->next) {
	GSList *n = n_lang->next;
	n_lang->next = NULL;
	n_tail->next = l;
	l = n;
      }

    l = g_slist_reverse(l);

    return(l);
}


static void
voice_list_free(GSList *l)
{
    GSList *tmp = l;

    while (tmp) {
        CORBA_free(tmp->data);
        tmp = tmp->next;
    }
    g_slist_free(l);
}


static GNOME_Speech_VoiceInfoList *
voice_info_list_from_voice_list(GSList *l)
{
    int i = 0;
    GNOME_Speech_VoiceInfoList *rv = GNOME_Speech_VoiceInfoList__alloc();

    if (!l) {
        rv->_length = 0;
        return rv;
    }

    rv->_length = rv->_maximum = g_slist_length(l);
    rv->_buffer = GNOME_Speech_VoiceInfoList_allocbuf(rv->_length);

    while (l) {
        GNOME_Speech_VoiceInfo *info =
                        (GNOME_Speech_VoiceInfo *) l->data;
        rv->_buffer[i].name = CORBA_string_dup (info->name);
        rv->_buffer[i].gender = info->gender;
        rv->_buffer[i].language = CORBA_string_dup (info->language);
        i++;
        l = l->next;
    }

    return(rv);
}


static GNOME_Speech_VoiceInfoList *
espeak_getVoices(PortableServer_Servant servant,
                 const GNOME_Speech_VoiceInfo *voice_spec,
                 CORBA_Environment *ev)
{
    GNOME_Speech_VoiceInfoList *rv;
    GSList *l;

    l = get_voice_list();
    rv = voice_info_list_from_voice_list(l);
    voice_list_free(l);

    return(rv);
}


static GNOME_Speech_VoiceInfoList *
espeak_getAllVoices(PortableServer_Servant servant,
                    CORBA_Environment *ev)
{
  GNOME_Speech_VoiceInfoList *rv;
    GSList *l;

    l = get_voice_list();
    rv = voice_info_list_from_voice_list(l);
    voice_list_free(l);

    return(rv);
}


static GNOME_Speech_Speaker
espeak_createSpeaker (PortableServer_Servant servant,
		      const GNOME_Speech_VoiceInfo *voice_spec,
		      CORBA_Environment *ev)
{
  EspeakSynthesisDriver *d = espeak_synthesis_driver_from_servant (servant);
  EspeakSpeaker *s = espeak_speaker_new (G_OBJECT(d), voice_spec);
  return CORBA_Object_duplicate(bonobo_object_corba_objref(BONOBO_OBJECT(s)), ev);
}



static void
espeak_synthesis_driver_init (EspeakSynthesisDriver *driver)
{
  driver->mutex = g_mutex_new ();
  driver->last_speaker = NULL;
  driver->utterance_queue = NULL;
}


static void
espeak_synthesis_driver_free_utterance (t_utterance *utterance)
{
  g_free ((void *) utterance->voice->name);
  g_free (utterance->voice);
  g_free (utterance->text);
  g_free (utterance);
}


static void
espeak_synthesis_driver_flush_queue (EspeakSynthesisDriver *driver)
{
  GSList *tmp;
	
  /* Flush the utterance queue */

  g_mutex_lock (driver->mutex);
  for (tmp = driver->utterance_queue; tmp; tmp = tmp->next) {
    t_utterance *u = (t_utterance *) tmp->data;
    espeak_synthesis_driver_free_utterance (u);
  }
  g_slist_free (driver->utterance_queue);
  driver->utterance_queue = NULL;
  g_mutex_unlock (driver->mutex);
}



static void
espeak_synthesis_driver_finalize (GObject *obj)
{
  EspeakSynthesisDriver *d = ESPEAK_SYNTHESIS_DRIVER (obj);

  espeak_Synchronize ();
  espeak_Terminate ();

  if (d->text_idle)
    g_source_remove (d->text_idle);

  espeak_synthesis_driver_flush_queue (d);

  if (parent_class->finalize)
    parent_class->finalize (obj);
  bonobo_main_quit ();
}



static void
espeak_synthesis_driver_class_init (EspeakSynthesisDriverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  
  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = espeak_synthesis_driver_finalize;

  /* Initialize parent class epv table */

  klass->epv._get_driverName = espeak__get_driverName;
  klass->epv._get_driverVersion = espeak__get_driverVersion;
  klass->epv._get_synthesizerName = espeak__get_synthesizerName;
  klass->epv._get_synthesizerVersion = espeak__get_synthesizerVersion;
  klass->epv.getVoices = espeak_getVoices;
  klass->epv.getAllVoices = espeak_getAllVoices;
  klass->epv.createSpeaker = espeak_createSpeaker;  
  klass->epv.driverInit = espeak_driverInit;
  klass->epv.isInitialized = espeak_isInitialized;
}




BONOBO_TYPE_FUNC_FULL (EspeakSynthesisDriver,
		       GNOME_Speech_SynthesisDriver,
		       bonobo_object_get_type (),
		       espeak_synthesis_driver);



EspeakSynthesisDriver *
espeak_synthesis_driver_new (void)
{
  EspeakSynthesisDriver *driver;
  driver = g_object_new (ESPEAK_SYNTHESIS_DRIVER_TYPE, NULL);
  return driver;
}




int
main (int argc,
      char **argv)
{
  EspeakSynthesisDriver *driver;
  char *obj_id;
  int ret;
  
  if (!bonobo_init (&argc, argv)) {
    g_error ("Could not initialize Bonobo Activation / Bonobo");
  }

  /* If threads haven't been initialized, initialize them */

  if (!g_threads_got_initialized)
    g_thread_init (NULL);
	
  obj_id = "OAFIID:GNOME_Speech_SynthesisDriver_Espeak:proto0.3";

  driver = espeak_synthesis_driver_new ();
  if (!driver)
    g_error ("Error creating speech synthesis driver object.\n");

  ret = bonobo_activation_active_server_register (
						  obj_id,
						  bonobo_object_corba_objref (bonobo_object (driver)));

  if (ret != Bonobo_ACTIVATION_REG_SUCCESS)
    g_error ("Error registering speech synthesis driver.\n");
  else {
    bonobo_main ();
  }
  return 0;
}



static gboolean
espeak_idle(gpointer data)
{
  EspeakSynthesisDriver *driver = ESPEAK_SYNTHESIS_DRIVER (data);
  gboolean rerun;

  g_mutex_lock (driver->mutex);

  if (driver->utterance_queue) {
    t_utterance *utterance;
    unsigned int unique_identifier=0;
    espeak_ERROR a_error = EE_INTERNAL_ERROR;

    utterance = (t_utterance *) driver->utterance_queue->data;
    espeak_SetVoiceByProperties(utterance->voice);      
    a_error = espeak_Synth((char *) utterance->text, 
			   strlen((char*)utterance->text) + 1, 
			   0, POS_CHARACTER, 0, espeakCHARS_UTF8,
			   &unique_identifier, 
			   utterance->user_data);

    if (a_error != EE_BUFFER_FULL) {
      driver->utterance_queue = g_slist_remove_link (
        driver->utterance_queue,
	driver->utterance_queue);
      espeak_synthesis_driver_free_utterance (utterance);
    }
  }

  if (driver->utterance_queue) {
    rerun = TRUE;
  } else {
    driver->text_idle = 0;
    rerun = FALSE;
  }
  
  g_mutex_unlock (driver->mutex);

  return rerun;
}


gint
espeak_synthesis_driver_say (EspeakSynthesisDriver *driver,
			     EspeakSpeaker *espeak_speaker,
			     gchar *text)
{
  t_utterance *utterance = NULL;
  Speaker *speaker = SPEAKER (espeak_speaker);

  speaker = SPEAKER (espeak_speaker);
  if (driver->last_speaker != espeak_speaker 
      || speaker_needs_parameter_refresh(speaker)) {
    speaker_refresh_parameters (speaker);
    driver->last_speaker = espeak_speaker;
  }

  utterance = g_new (t_utterance, 1);
  utterance->voice = g_new (espeak_VOICE, 1);
  memset(utterance->voice, 0, sizeof(espeak_VOICE));
  utterance->voice->name = g_strdup (espeak_speaker->voice.name);
  utterance->voice->gender = espeak_speaker->voice.gender;
  utterance->text = g_strdup (text);

  if (speaker->cb != CORBA_OBJECT_NIL) {
    t_user_data *user_data;
    CORBA_Environment ev;
    user_data = g_new(t_user_data, 1);
    CORBA_exception_init (&ev);
    user_data->cb = CORBA_Object_duplicate (speaker->cb, &ev);
    CORBA_exception_free (&ev);
    user_data->speech_is_started = FALSE;
    user_data->id = ++utterance_id;
    utterance->user_data = user_data;
  } else {
    utterance->user_data = NULL;
  }

  g_mutex_lock (driver->mutex);
  driver->utterance_queue = g_slist_append (driver->utterance_queue, 
					    utterance);
  if (!driver->text_idle) {
      driver->text_idle = g_idle_add (espeak_idle, driver);
  }
  g_mutex_unlock (driver->mutex);

  return utterance_id;
}


gboolean
espeak_synthesis_driver_stop (EspeakSynthesisDriver *d)
{
  if (d->text_idle) {
    g_source_remove (d->text_idle);
    d->text_idle = 0;
  }

  espeak_synthesis_driver_flush_queue (d);

  espeak_Cancel ();
  return TRUE;
}


void
espeak_synthesis_driver_wait (EspeakSynthesisDriver *d)
{
  espeak_Synchronize ();
}


gboolean
espeak_synthesis_driver_is_speaking (EspeakSynthesisDriver *d)
{
  return espeak_IsPlaying ();
}


gboolean
espeak_synthesis_driver_set_voice_spec (const EspeakSynthesisDriver *d,
					espeak_VOICE *voice_spec)
{
  espeak_ERROR error = espeak_SetVoiceByProperties(voice_spec);
  return (EE_OK == error);
}

gboolean
espeak_synthesis_driver_set_voice_name (const EspeakSynthesisDriver *d,
					gchar* name)
{
  espeak_ERROR error = espeak_SetVoiceByName(name);
  return (error == EE_OK);
}

gboolean
espeak_synthesis_driver_set_param (const EspeakSynthesisDriver *d,
				   espeak_PARAMETER param,
				   gint new_value)
{
  espeak_ERROR error = espeak_SetParameter (param, new_value, 0);
  return (error == EE_OK);
}


