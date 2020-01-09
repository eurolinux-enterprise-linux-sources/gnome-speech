/*
 * GNOME Speech - Speech services for the GNOME desktop
 *
 * Copyright 2002 Sun Microsystems Inc.
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
 * loquendosynthesisdriver.c: Implements a GNOME Speech driver for Loquendo TTS
 *
 */

#include <libbonobo.h>
#include <glib/gmain.h>
#include <loqtts.h>
#include <gnome-speech/gnome-speech.h>
#include "loquendosynthesisdriver.h"
#include "loquendospeaker.h"
#include <unistd.h> 	

#define VERSION_LENGTH 20

typedef struct {
	GNOME_Speech_SpeechCallback cb;
	GNOME_Speech_speech_callback_type type;
	gint text_id;
	gint offset;
} index_queue_entry;

static gint text_id = 0;
static GObjectClass *parent_class;


//////////////////////////
/// Aux functions to manage indexes
//////////////////////////

static void index_queue_entry_destroy (index_queue_entry *e)
{
	CORBA_Environment ev;
	g_return_if_fail (e);
	CORBA_exception_init (&ev);
	CORBA_Object_release (e->cb, &ev);
	CORBA_exception_free (&ev);
	g_free (e);
}

static void loquendo_synthesis_driver_flush_index_queue (LoquendoSynthesisDriver *d)
{
	GSList *tmp;
	g_mutex_lock (d->mutex);
	for (tmp = d->index_queue; tmp; tmp = tmp->next)
	{
		index_queue_entry *e = (index_queue_entry *) tmp->data;
		index_queue_entry_destroy (e);
	}
	g_slist_free (d->index_queue);
	d->index_queue = NULL;
	g_mutex_unlock (d->mutex);
}

	
static void loquendo_add_string (LoquendoSynthesisDriver *d,  gchar *text)
{
	gchar *new_string;

	if (d->cadena) 
	{
		new_string = g_strconcat (d->cadena, text, NULL);
		g_free (d->cadena);
		d->cadena = new_string;
	}
	else
		{
			d->cadena = g_strdup (text);
		}

}
	

static void loquendo_synthesis_driver_add_index (LoquendoSynthesisDriver *d,
				     GNOME_Speech_SpeechCallback cb,
				     GNOME_Speech_speech_callback_type type,
				     gint text_id,
				     gint offset)
{
	index_queue_entry *e;
	CORBA_Environment ev;
	gchar *marker;

	e = g_new(index_queue_entry, 1);
	CORBA_exception_init (&ev);
	e->cb = CORBA_Object_duplicate (cb, &ev);
	CORBA_exception_free (&ev);
	e->type = type;
	e->text_id = text_id;
	e->offset = offset;

	g_mutex_lock (d->mutex);
	d->index_queue = g_slist_append(d->index_queue, e);
	g_mutex_unlock (d->mutex);

	if ( type==GNOME_Speech_speech_callback_speech_progress)
	{
		marker = g_strdup ("\\k1 ");
		loquendo_add_string (d, marker);
		g_free (marker);
	}
}




static GSList *get_voice_list()
{
	GSList *l = NULL;
	gint i;
	gchar qr[1000];

	gint numVoces = ttsQuery(NULL,"Speaker,Gender,Language","", qr, 1000, ttsFALSE);

	gchar *nombreVoz=strtok(qr,";");

	for (i = 0; i < numVoces; i++) 
	{

		GNOME_Speech_VoiceInfo *info = GNOME_Speech_VoiceInfo__alloc();

		gchar *generoVoz=strtok(NULL, ";");
		gchar *idiomaVoz=strtok(NULL, ";");

		info->language = CORBA_string_dup(idiomaVoz);
		info->name = CORBA_string_dup(nombreVoz);
		if (!strcmp (generoVoz, "Male"))
			info->gender = GNOME_Speech_gender_male;
		else if (!strcmp (generoVoz, "Female"))
			info->gender = GNOME_Speech_gender_female;
		else
			info->gender=-1;

		l = g_slist_prepend(l, info);

		nombreVoz=strtok(NULL,";");
	}
	l = g_slist_reverse(l);

	return(l);
}

static gboolean lang_matches_spec (gchar *lang, gchar *spec)
{
	gboolean lang_matches, country_matches = FALSE, encoding_matches = TRUE;

	if (!strcmp (lang, spec))
	{
		return TRUE;
	}
	else
	{
		gchar *lang_country_cp, *spec_country_cp;
		gchar *lang_encoding_cp, *spec_encoding_cp;
		lang_matches = !strncmp (lang, spec, 2);
		if (lang_matches) 
		{ 
			lang_country_cp = strchr (lang, '_');
			spec_country_cp = strchr (spec, '_');
			lang_encoding_cp = strchr (lang, '@');
			spec_encoding_cp = strchr (spec, '@');

			if (!lang_country_cp || !spec_country_cp)
			{
				country_matches = TRUE;
			}
			else
			{
				country_matches = !strncmp (lang_country_cp, spec_country_cp, 3);
			}
			if (spec_encoding_cp)
			{
				if (lang_encoding_cp)
				{
					encoding_matches = !strcmp (spec_encoding_cp, lang_encoding_cp); 
				}
				else
				{
					encoding_matches = FALSE;
				}
			}
		}
		return lang_matches && country_matches && encoding_matches;
	}
}

static GSList *prune_voice_list (GSList *l,  const GNOME_Speech_VoiceInfo *info)
{
	GSList *cur, *next;
	GNOME_Speech_VoiceInfo *i;

	cur = l;
	while (cur) {
		i = (GNOME_Speech_VoiceInfo *) cur->data;
		next = cur->next;
		if (strlen(info->name))
			if (strcmp (i->name, info->name)) {
				CORBA_free (i);
				l = g_slist_remove_link (l, cur);
				cur = next;
				continue;
			}
		if (strlen(info->language))
			if (!lang_matches_spec (i->language, info->language)) {
				CORBA_free (i);
				l = g_slist_remove_link (l, cur);
				cur = next;
				continue;
			}
		if ((info->gender == GNOME_Speech_gender_male || info->gender == GNOME_Speech_gender_female) &&    (info->gender != i->gender)) {
			CORBA_free (i);
			l = g_slist_remove_link (l, cur);
			cur = next;
			continue;
		}
		cur = next;
	}
	return l;
}



static GNOME_Speech_VoiceInfoList *voice_info_list_from_voice_list (GSList *l)
{
	int i = 0;
	GNOME_Speech_VoiceInfoList *rv = GNOME_Speech_VoiceInfoList__alloc();
  

	if (!l) {
		rv->_length = 0;
		return (rv);
	}


	rv->_length = rv->_maximum = g_slist_length(l);
	rv->_buffer = GNOME_Speech_VoiceInfoList_allocbuf(rv->_length);
	while (l) {
		GNOME_Speech_VoiceInfo *info = (GNOME_Speech_VoiceInfo *) l->data;

		rv->_buffer[i].name = CORBA_string_dup(info->name);
		rv->_buffer[i].gender = info->gender;
		rv->_buffer[i].language = CORBA_string_dup(info->language);

		i++;
		l = l->next;

	}
	return (rv);
}

static void voice_list_free(GSList *l)
{
	GSList *tmp = l;

	while (tmp) {
		CORBA_free(tmp->data);
		tmp = tmp->next;
	}
	g_slist_free(l);
}



static LoquendoSynthesisDriver * loquendo_synthesis_driver_from_servant (PortableServer_Servant *servant)
{
	return LOQUENDO_SYNTHESIS_DRIVER(bonobo_object_from_servant (servant));
}


static CORBA_string loquendo__get_driverName (PortableServer_Servant servant,CORBA_Environment * ev)
{
	return CORBA_string_dup ("Loquendo TTS GNOME Speech Driver");
}


static CORBA_string loquendo__get_synthesizerName (PortableServer_Servant servant,CORBA_Environment * ev)
{
	return CORBA_string_dup ("Loquendo TTS");
}


static CORBA_string loquendo__get_driverVersion (PortableServer_Servant aservant,CORBA_Environment * ev)
{
	return CORBA_string_dup ("0.1");
}

static CORBA_string loquendo__get_synthesizerVersion (PortableServer_Servant servant,CORBA_Environment * ev)
{
	ttsInfoStringType LTTSversion;	
	ttsGetVersionInfo(LTTSversion);
	return CORBA_string_dup (LTTSversion);
}


static GNOME_Speech_VoiceInfoList * loquendo_getVoices(PortableServer_Servant servant,
								   const GNOME_Speech_VoiceInfo *voice_spec,
								   CORBA_Environment *ev)
{
	GNOME_Speech_VoiceInfoList *rv;
	GSList *l;

	l = get_voice_list();
	l = prune_voice_list (l, voice_spec);
	rv = voice_info_list_from_voice_list(l);
	voice_list_free(l);

	return(rv);
}



static GNOME_Speech_VoiceInfoList * loquendo_getAllVoices (PortableServer_Servant servant,
														   CORBA_Environment *ev)
{
	GNOME_Speech_VoiceInfoList *rv;
	GSList *l;

	l = get_voice_list();
	rv = voice_info_list_from_voice_list(l);
	voice_list_free(l);  
	return(rv);
}


static CORBA_boolean loquendo_isInitialized (PortableServer_Servant servant,CORBA_Environment *ev)
{
	LoquendoSynthesisDriver *driver = loquendo_synthesis_driver_from_servant (servant);
	return driver->initialized;
}

static gboolean loquendo_synthesis_driver_timeout_callback (void *data)
{
//	LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER (data);
//	return ttsDone (d->handle);
	return TRUE;
} 


static gboolean loquendo_synthesis_driver_callback_function(ttsEventType nReason,
																	void *lData,
																	void *pUser)
{
   LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER(pUser);
   index_queue_entry *e = NULL;
   CORBA_Environment ev;
   g_mutex_lock (d->mutex);
   if (d && d->index_queue)
	{
      e = (index_queue_entry *) d->index_queue->data;
      d->index_queue = g_slist_remove_link (d->index_queue, d->index_queue);

    }

   g_mutex_unlock (d->mutex);

   switch (nReason) 
	{
		case TTSEVT_AUDIOSTART:
		{
			if (e)
			{
				CORBA_exception_init (&ev);
				GNOME_Speech_SpeechCallback_notify (e->cb,
						    GNOME_Speech_speech_callback_speech_started,
						    e->text_id,
						    0,
						    &ev);
				CORBA_Object_release (e->cb, &ev);
				CORBA_exception_free (&ev);
				g_free (e);
				e = NULL;
			}

			break;
		}
		case TTSEVT_ENDOFSPEECH:
		{
			if (e)
 			{
				CORBA_exception_init (&ev);
				GNOME_Speech_SpeechCallback_notify (e->cb,
						    GNOME_Speech_speech_callback_speech_ended,
						    e->text_id,
						    0,
						    &ev);
				CORBA_Object_release (e->cb, &ev);
				CORBA_exception_free (&ev);
				g_free (e);
				e = NULL;
			}

			break;
		}
		case TTSEVT_BOOKMARK:
		{
			if (e) 
			{
				CORBA_exception_init (&ev);
				GNOME_Speech_SpeechCallback_notify (e->cb,
							    GNOME_Speech_speech_callback_speech_progress,
							    e->text_id,
							    e->offset,
							    &ev);
				CORBA_Object_release (e->cb, &ev);
				CORBA_exception_free (&ev);
				g_free (e);
				e = NULL;
			}

			break;
		}
		default:
		{
			break;
		}
	}

	return TRUE;
}


static CORBA_boolean loquendo_driverInit (PortableServer_Servant servant,CORBA_Environment *ev)
{
  ttsHandleType hVoice;		/* Voice handle */
  ttsResultType err;		/* Error code returned by TTS APIs */

	LoquendoSynthesisDriver *d = loquendo_synthesis_driver_from_servant (servant);
	
	if (!d->initialized)
	{

    /* Initializes the LoquendoTTS Instance */
	  err = ttsNewInstance(&d->handle, NULL, "/etc/default.session");
	  if (err != tts_OK)
	  {
	    fprintf(stderr, "%s", ttsGetError(NULL));
	    return FALSE;
	  }

	  err = ttsNewVoice(&hVoice, d->handle, "Jorge", 16000, "l");
	  if (err != tts_OK)
	  {
		 fprintf(stderr, "%s", ttsGetError(d->handle));
   	 (void)ttsDeleteInstance(d->handle);
	    return FALSE;
	  }

	  err = ttsSetAudio(d->handle, "LoqAudioBoard", NULL, "l", 0);
	  if (err != tts_OK)
	  {
		 fprintf(stderr, "%s", ttsGetError(d->handle));
	    (void)ttsDeleteInstance(d->handle);
	    return FALSE;
	  }

    ttsEnableEvent( d->handle, TTSEVT_TEXT, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_WORDTRANSCRIPTION, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_PHONEMES, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_DATA, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_TAG, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_NOTSENT, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_FREESPACE, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_ERROR, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_SENTENCE, FALSE);
    ttsEnableEvent( d->handle, TTSEVT_AUDIO, FALSE);


		/* Add a timeout callback to poke this instance of Loquendo */
		d->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH_IDLE, 1000,  
				                     			loquendo_synthesis_driver_timeout_callback, d, NULL);

		ttsRegisterCallback (d->handle,loquendo_synthesis_driver_callback_function,
			                (void *) d, 0);
		d->initialized = TRUE;
	}
	return d->initialized;
}



static GNOME_Speech_Speaker loquendo_createSpeaker (PortableServer_Servant servant,
													const GNOME_Speech_VoiceInfo *voice_spec,
													CORBA_Environment *ev)
{
	LoquendoSynthesisDriver *d = loquendo_synthesis_driver_from_servant (servant);
	LoquendoSpeaker *s = loquendo_speaker_new (G_OBJECT(d), voice_spec);
	return CORBA_Object_duplicate(bonobo_object_corba_objref(BONOBO_OBJECT(s)), ev);
}




static void loquendo_synthesis_driver_init (LoquendoSynthesisDriver *driver)
{
	driver->mutex = g_mutex_new ();
	driver->last_speaker = NULL;
	driver->index_queue = NULL;
	driver->timeout_id = -1;
	driver->initialized=FALSE;
	driver->cadena=NULL;
}



static void loquendo_synthesis_driver_finalize (GObject *obj)
{
	LoquendoSynthesisDriver *d = LOQUENDO_SYNTHESIS_DRIVER (obj);

	if (d->handle != NULL)
	{
		loquendo_synthesis_driver_wait(d);
		ttsDeleteInstance (d->handle);
	}

	/* Remove timeout */
	if (d->timeout_id >= 0)
	{
		g_source_remove (d->timeout_id);
	}

	/* Flush the index queue */

	loquendo_synthesis_driver_flush_index_queue (d);

	if (parent_class->finalize)
	{
		parent_class->finalize (obj);
	}
	bonobo_main_quit ();
}




static void loquendo_synthesis_driver_class_init (LoquendoSynthesisDriverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
  
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = loquendo_synthesis_driver_finalize;

	/* Initialize parent class epv table */

	klass->epv._get_driverName = loquendo__get_driverName;
	klass->epv._get_driverVersion = loquendo__get_driverVersion;
	klass->epv._get_synthesizerName = loquendo__get_synthesizerName;
	klass->epv._get_synthesizerVersion = loquendo__get_synthesizerVersion;
	klass->epv.getVoices = loquendo_getVoices;
	klass->epv.getAllVoices = loquendo_getAllVoices;
	klass->epv.createSpeaker = loquendo_createSpeaker;  
	klass->epv.driverInit = loquendo_driverInit;
	klass->epv.isInitialized = loquendo_isInitialized;
}


gint loquendo_synthesis_driver_say (LoquendoSynthesisDriver *d,LoquendoSpeaker *s,gchar *text)
{
	Speaker *speaker = SPEAKER (s);
	gchar *beginning = text;
	gchar *end, *word;
	ttsResultType r;	

	g_assert (g_utf8_validate (text, -1, NULL));

  /* If this speaker wasn't the last one to speak, reset the speech parameters */
	if (d->last_speaker != s || speaker_needs_parameter_refresh(speaker)) 
	{
		speaker_refresh_parameters (speaker);
		d->last_speaker = s;
	}
	
	text_id++;

	if (speaker->cb != CORBA_OBJECT_NIL) 
	{
		/// La primera marca es la de inicio de reproduccion
		loquendo_synthesis_driver_add_index(d,
						   speaker->cb,
						   GNOME_Speech_speech_callback_speech_started,
						   text_id, 0);

		// Add index between words 
		while (*beginning) 
		{
			///entre cada palabra (en el espacio en blanco) inserto una marca 
			end = beginning;
			while (*end && g_unichar_isspace(g_utf8_get_char (end)))
				end = beginning = g_utf8_next_char (end);
			while (*end && !g_unichar_isspace(g_utf8_get_char (end)))
				end = g_utf8_next_char (end);
			word = g_strndup (beginning, end-beginning);
			loquendo_add_string (d, word);

			g_free (word);
			loquendo_synthesis_driver_add_index(d,
						   speaker->cb,
						   GNOME_Speech_speech_callback_speech_progress,
						   text_id, end-text);

			if (!*end)
				break;
			beginning = g_utf8_next_char (end);
		}

		loquendo_synthesis_driver_add_index(d,
						   speaker->cb,
						   GNOME_Speech_speech_callback_speech_ended,
						   text_id, 0);

	}
	else
	{
		loquendo_add_string(d, text);
	}

	/* In the absence of text, we'll force loquendo to do
	 * something so we'll get the proper set of callbacks.
	 * The '' in this case speaks nothing but accomplishes
	 * the goal of making the callbacks work.
	 */
	if (!d->cadena)
	  loquendo_add_string(d, "''");

   r = ttsRead( d->handle,d->cadena,TTSBUFFER,TTSUTF8,TTSDEFAULT,TTSNONBLOCKING);

	g_free (d->cadena);
	d->cadena = NULL;

	if (tts_OK==r)
		return text_id;
	else
		return -1;
}


gboolean loquendo_synthesis_driver_stop (LoquendoSynthesisDriver *d)
{
	loquendo_synthesis_driver_flush_index_queue (d);

	if (d->cadena)
	 {
		g_free (d->cadena);
		d->cadena = NULL;
	}

	ttsStop (d->handle);
	return TRUE;
}


void loquendo_synthesis_driver_wait (LoquendoSynthesisDriver *d)
{

	while (!ttsDone(d->handle)) 
	{
		usleep(1000);
	};
}


gboolean loquendo_synthesis_driver_is_speaking (LoquendoSynthesisDriver *d)
{
	return ttsDone (d->handle);
}


BONOBO_TYPE_FUNC_FULL (LoquendoSynthesisDriver,GNOME_Speech_SynthesisDriver,
					   bonobo_object_get_type (),loquendo_synthesis_driver);


LoquendoSynthesisDriver * loquendo_synthesis_driver_new (void)
{
	LoquendoSynthesisDriver *driver;
	driver = g_object_new (LOQUENDO_SYNTHESIS_DRIVER_TYPE, NULL);
	return driver;
}


int main (int argc, char **argv)
{
	LoquendoSynthesisDriver *driver;
	char *obj_id;
	int ret;


	if (!bonobo_init (&argc, argv))
	{
		g_error ("Could not initialize Bonobo Activation / Bonobo");
	}
	/* If threads haven't been initialized, initialize them */
	if (!g_threads_got_initialized)
		g_thread_init (NULL);
	
	obj_id = "OAFIID:GNOME_Speech_SynthesisDriver_Loquendo:proto0.3";

	driver = loquendo_synthesis_driver_new ();
	if (!driver)
		g_error ("Error creating speech synthesis driver object.\n");

	ret = bonobo_activation_active_server_register (
                obj_id,
                bonobo_object_corba_objref (bonobo_object (driver)));
	if (ret != Bonobo_ACTIVATION_REG_SUCCESS)
		g_error ("Error registering speech synthesis driver.\n");
	else 
	{
		bonobo_main ();
	}

	return 0;
}

