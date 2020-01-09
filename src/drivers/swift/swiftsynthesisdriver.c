/*
 *
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
 * swiftsynthesisdriver.c: Implements the SwiftSynthesisDriver object--
 *                    a GNOME Speech driver for Cepstral's Swift TTS engine
 *
 */

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libbonobo.h>
#include <glib/gmain.h>
#include <gnome-speech/gnome-speech.h>
#include <swift.h>
#include "swiftsynthesisdriver.h"

static gint text_id = 0;

typedef struct {
	Speaker            *speaker;
	swift_port         *tts_port;
	swift_voice        *tts_voice;
        swift_params       *tts_params;
	swift_background_t tts_async;
	gchar              *text;
	gint               text_id;
} utterance;

static GObjectClass *parent_class;

static void
utterance_destroy (utterance *u)
{
	g_return_if_fail (u);
	if (u->text) {
		g_free (u->text);
	}
	if (u->speaker) {
	        g_object_unref (u->speaker);
	}
	g_free (u);
}

static void
utterance_list_destroy (GSList *l)
{
	GSList *tmp;

	for (tmp = l; tmp; tmp = tmp->next) {
		utterance_destroy ((utterance *) tmp->data);
	}

	g_slist_free (l);
}

static SwiftSynthesisDriver *
swift_synthesis_driver_from_servant (PortableServer_Servant *servant)
{
	return SWIFT_SYNTHESIS_DRIVER(bonobo_object_from_servant(servant));
}

static CORBA_string
swift__get_driverName(PortableServer_Servant servant,
                        CORBA_Environment * ev)
{
	return(CORBA_string_dup("Swift GNOME Speech Driver"));
}

static CORBA_string
swift__get_synthesizerName(PortableServer_Servant servant,
                             CORBA_Environment * ev)
{
	return(CORBA_string_dup("Cepstral Swift"));
}

static CORBA_string
swift__get_driverVersion(PortableServer_Servant aservant,
                           CORBA_Environment * ev)
{
	return(CORBA_string_dup("1.0"));
}

static CORBA_string
swift__get_synthesizerVersion(PortableServer_Servant servant,
                                CORBA_Environment * ev)
{
	return(CORBA_string_dup(swift_version));
}

static GSList *
get_voice_list(PortableServer_Servant servant)
{
	SwiftSynthesisDriver *d = 
                swift_synthesis_driver_from_servant (servant);

	GSList *l = NULL;

	swift_voice *voice;
	
	voice = swift_port_find_first_voice (d->tts_port, NULL, NULL);
	while (voice) {
		const char *lang;
		const char *raw_gender;
		GString *gender;

		GNOME_Speech_VoiceInfo *info = GNOME_Speech_VoiceInfo__alloc();

		lang = swift_voice_get_attribute (voice, "language/tag");
		info->language = CORBA_string_dup (lang);
		info->name = CORBA_string_dup(
		        swift_voice_get_attribute (voice, "name"));
                raw_gender = swift_voice_get_attribute (voice, "speaker/gender");
		gender = g_string_ascii_down(g_string_new(raw_gender));
 		if (!strcmp (gender->str, "male"))
			info->gender = GNOME_Speech_gender_male;
		else if (!strcmp (gender->str, "female"))
			info->gender = GNOME_Speech_gender_female;
		else
			info->gender = GNOME_Speech_gender_male;
		g_string_free(gender, TRUE);
		l = g_slist_prepend (l, info);
		voice = swift_port_find_next_voice (d->tts_port);
	}
	l = g_slist_reverse(l);
	return l;
}


static void
voice_list_free(GSList *l)
{
	GSList *tmp = l;

	while (tmp) {
		CORBA_free (tmp->data);
		tmp = tmp->next;
	}
	g_slist_free(l);
}

static gboolean
lang_matches_spec (gchar *lang, gchar *spec)
{
	gboolean lang_matches;
	gboolean country_matches = FALSE;
	gboolean encoding_matches = TRUE;

	if (!strcmp (lang, spec)) {
		return TRUE;
	} else {
		gchar *lang_country_cp, *spec_country_cp;
		gchar *lang_encoding_cp, *spec_encoding_cp;
		lang_matches = !strncmp (lang, spec, 2);
		if (lang_matches) {
			lang_country_cp = strchr (lang, '_');
			spec_country_cp = strchr (spec, '_');
			lang_encoding_cp = strchr (lang, '@');
			spec_encoding_cp = strchr (spec, '@');

			if (!lang_country_cp || !spec_country_cp) {
				country_matches = TRUE;
			} else {
				country_matches = 
				        !strncmp (lang_country_cp, 
						  spec_country_cp, 
						  3);
			}

			if (spec_encoding_cp) {
				if (lang_encoding_cp) {
					encoding_matches = 
					        !strcmp (spec_encoding_cp, 
							 lang_encoding_cp);
				} else {
					encoding_matches = FALSE;
				}
			}
		}
		return lang_matches && country_matches && encoding_matches;
	}
}

static GSList *
prune_voice_list (GSList *l,
		  const GNOME_Speech_VoiceInfo *info)
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
		if ((info->gender == GNOME_Speech_gender_male 
		     || info->gender == GNOME_Speech_gender_female) 
		    && (info->gender != i->gender)) {
			CORBA_free (i);
			l = g_slist_remove_link (l, cur);
			cur = next;
			continue;
		}
		cur = next;
	}
	return l;
}

static GNOME_Speech_VoiceInfoList *
voice_info_list_from_voice_list (GSList *l)
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

		rv->_buffer[i].name = CORBA_string_dup(info->name);
		rv->_buffer[i].gender = info->gender;
		rv->_buffer[i].language = CORBA_string_dup(info->language);
		i++;
		l = l->next;
	}

	return rv;
}

static gboolean
swift_timeout (gpointer data)
{
	SwiftSynthesisDriver *d = SWIFT_SYNTHESIS_DRIVER (data);
	utterance *u;
	CORBA_Environment ev;
	CORBA_long offset = 0;
	gint rv;

	/* Storage for current utterance values 
	 */
	swift_status_t status;
	Speaker *s;
	gint text_id;

	CORBA_exception_init (&ev);

        /* Check to see if the speech process has sent us any notifiations 
	 */
	rv = read (d->status_pipe[0], &offset, sizeof(gint));
	if (rv == sizeof(gint)) {
		u = (utterance *) d->utterances->data;
		if (u->speaker->cb != CORBA_OBJECT_NIL) {
			GNOME_Speech_SpeechCallback_notify (
			       u->speaker->cb,
			       GNOME_Speech_speech_callback_speech_progress,
			       u->text_id,
			       offset,
			       &ev);
		}
	}

	if (d->speaking) {
		u = (utterance *) d->utterances->data;
		status = swift_port_status (d->tts_port,
					    u->tts_async);
		if ((status == SWIFT_STATUS_RUNNING)
		    || (status == SWIFT_STATUS_QUEUED)) {
		        return TRUE;
		}
		swift_port_stop (d->tts_port, 
				 u->tts_async, 
				 SWIFT_EVENT_NOW);
		d->utterances = g_slist_remove (d->utterances, u);
		text_id = u->text_id;
		s = g_object_ref (u->speaker);
		utterance_destroy (u);
		d->speaking = FALSE;
		if (s->cb != CORBA_OBJECT_NIL) {
			GNOME_Speech_SpeechCallback_notify (
		                s->cb,
				GNOME_Speech_speech_callback_speech_ended,
				text_id,
				0,
				&ev);
		}
		g_object_unref (s);
	}

	if (!d->utterances) {
		d->timeout_id = 0;
		return FALSE;
	}

	u = (utterance *) d->utterances->data;
	swift_port_set_voice (u->tts_port, u->tts_voice);
	swift_port_speak_text (u->tts_port, 
			       u->text, 
			       0, 
			       SWIFT_UTF8,
			       &u->tts_async,
	                       u->tts_params);	

	d->speaking = TRUE;
	if (u->speaker->cb != CORBA_OBJECT_NIL) {
		GNOME_Speech_SpeechCallback_notify (
		        u->speaker->cb,
			GNOME_Speech_speech_callback_speech_started,
			u->text_id,
			0,
			&ev);
	}
	CORBA_exception_free (&ev);
	return TRUE;
}

swift_result_t 
swift_event_callback (swift_event *event,
		      swift_event_t type,
		      void *udata)
{
	SwiftSynthesisDriver *d = SWIFT_SYNTHESIS_DRIVER (udata);
	utterance *u;
	gint offset, l;

	if (!d->utterances) {
	        return SWIFT_SUCCESS;
	}

	u = (utterance *) d->utterances->data;

	switch (type) {
	case SWIFT_EVENT_START:
		break;
	case SWIFT_EVENT_END:
		break;
	case SWIFT_EVENT_TOKEN:
	        swift_event_get_textpos(event, &offset, &l);
		write (d->status_pipe[1], &offset, sizeof(gint));
		break;
	default:
	        break;
	}

	return SWIFT_SUCCESS;
}

static void
swift_synthesis_driver_set_timeout (SwiftSynthesisDriver *d)
{
        if (!d->timeout_id) {
		d->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH,
						    50,
						    swift_timeout,
						    (gpointer) d, 
						    NULL);
	}
}

static CORBA_boolean
swift_driverInit (PortableServer_Servant servant,
		    CORBA_Environment *ev)
{
 	SwiftSynthesisDriver *d = 
                swift_synthesis_driver_from_servant (servant);

	if (d->initialized)
		return TRUE;
	if (pipe (d->status_pipe))
		return FALSE;
	fcntl (d->status_pipe[0], F_SETFL, O_NONBLOCK);
	fcntl (d->status_pipe[1], F_SETFL, O_NONBLOCK);

	d->tts_engine = swift_engine_open (NULL);
        if (!d->tts_engine) {
	        g_error ("Could not open swift engine.\n");
		return FALSE;
	}

	d->tts_port = swift_port_open (d->tts_engine, NULL);
	if (!d->tts_port) {
	        g_error ("Could not open swift port.\n");
		return FALSE;
	}

	/* Disable SSML processing, which is what is used by default. 
	 */
	swift_port_set_param_string (d->tts_port, 
				     "tts/content-type",
				     "text/plain",
				     SWIFT_ASYNC_NONE);

	swift_port_set_callback(d->tts_port, 
				swift_event_callback, 
				SWIFT_EVENT_START
				| SWIFT_EVENT_TOKEN
				| SWIFT_EVENT_END, 
				d);

	d->initialized = TRUE;
	return TRUE;
}

static CORBA_boolean
swift_driver_isInitialized (PortableServer_Servant servant,
			    CORBA_Environment *ev)
{
	SwiftSynthesisDriver *d = 
	        swift_synthesis_driver_from_servant (servant);
 	return d->initialized;
}

static GNOME_Speech_VoiceInfoList *
swift_driver_getVoices(PortableServer_Servant servant,
		       const GNOME_Speech_VoiceInfo *voice_spec,
		       CORBA_Environment *ev)
{
	GNOME_Speech_VoiceInfoList *rv;
	GSList *l;

	l = get_voice_list (servant);
	l = prune_voice_list (l, voice_spec);
	rv = voice_info_list_from_voice_list (l);
	voice_list_free (l);

	return rv;
}

static GNOME_Speech_VoiceInfoList *
swift_driver_getAllVoices(PortableServer_Servant servant,
			  CORBA_Environment *ev)
{
	GNOME_Speech_VoiceInfoList *rv;
	GSList *l;

	l = get_voice_list (servant);
	rv = voice_info_list_from_voice_list (l);
	voice_list_free (l);

	return (rv);
}

static GNOME_Speech_Speaker
swift_driver_createSpeaker(PortableServer_Servant servant,
			   const GNOME_Speech_VoiceInfo *voice_spec,
			   CORBA_Environment *ev)
{
	SwiftSynthesisDriver *d = 
                swift_synthesis_driver_from_servant (servant);

	SwiftSpeaker *s;
	GSList *l;
	GNOME_Speech_VoiceInfo *info;

	l = get_voice_list (servant);
	l = prune_voice_list (l, voice_spec);

	if (l) {
		info = l->data;
	} else {
		info = NULL;
	}

	s = swift_speaker_new (G_OBJECT(d), info);

	return(CORBA_Object_duplicate(bonobo_object_corba_objref(
				              BONOBO_OBJECT(s)), 
				      ev));
}

gint
swift_synthesis_driver_say (SwiftSynthesisDriver *d,
			    SwiftSpeaker         *s,
			    const gchar          *text)
{
        /* [[[WDW - from the docs: "After the synthesis process 
         * represented by a swift_background_t has completed (you can
         * determine this by calling swift_port_status()), you MUST call
         * swift_port_stop() on it to free up the resources used by
         * it. You can also force synthesis to stop with this function.]]]
	 */
	utterance *u;

	swift_synthesis_driver_set_timeout (d);

	u = g_new (utterance, 1);
	u->tts_port   = d->tts_port;
	u->tts_voice  = s->tts_voice;
	u->tts_params = swift_params_new(NULL);
	swift_params_set_params(u->tts_params, s->tts_params);
	u->tts_async  = NULL;
	u->text_id    = text_id++;
	u->text       = g_strdup (text);
	u->speaker    = g_object_ref (s);
	d->utterances = g_slist_append (d->utterances, u);
	return u->text_id;
}

gboolean
swift_synthesis_driver_stop (SwiftSynthesisDriver *d)
{
	gint offset;

	while (read (d->status_pipe[0], 
		     &offset, 
		     sizeof(gint)) 
	       == sizeof(gint));

	if (!d->utterances) {
		return FALSE;
	}

	swift_port_stop (d->tts_port, SWIFT_ASYNC_ANY, SWIFT_EVENT_NOW);

	utterance_list_destroy (d->utterances);
	d->utterances = NULL;

	d->speaking = FALSE;
	if (d->timeout_id) {
		g_source_remove (d->timeout_id);
		d->timeout_id = 0;
	}

	return TRUE;
}

gboolean
swift_synthesis_driver_is_speaking (SwiftSynthesisDriver *d)
{
        swift_status_t status;

	if (d->utterances) {
		status = swift_port_status (d->tts_port, SWIFT_ASYNC_ANY);
		return ((status == SWIFT_STATUS_RUNNING) 
			|| (status == SWIFT_STATUS_QUEUED));
	} else {
		return FALSE;
	}
}

static void
swift_synthesis_driver_init(SwiftSynthesisDriver *driver)
{
	driver->utterances     = NULL;
	driver->speaking       = FALSE;
	driver->status_pipe[0] = driver->status_pipe[1] = -1;
	driver->timeout_id     = 0;
	driver->initialized    = FALSE;
}

static void
swift_synthesis_driver_finalize(GObject *obj)
{
	SwiftSynthesisDriver *d = SWIFT_SYNTHESIS_DRIVER (obj);

	swift_port_stop (d->tts_port, SWIFT_ASYNC_ANY, SWIFT_EVENT_NOW);
	swift_port_close (d->tts_port);
	swift_engine_close (d->tts_engine);

	if (d->status_pipe[0] != -1)
		close (d->status_pipe[0]);
	if (d->status_pipe[1] != -1)
		close (d->status_pipe[1]);
	if (parent_class->finalize)
		parent_class->finalize(obj);

	bonobo_main_quit ();
}

static void
swift_synthesis_driver_class_init(SwiftSynthesisDriverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = swift_synthesis_driver_finalize;

	/* Initialize parent class epv table 
	 */
	klass->epv._get_driverName         = swift__get_driverName;
	klass->epv._get_driverVersion      = swift__get_driverVersion;
	klass->epv._get_synthesizerName    = swift__get_synthesizerName;
	klass->epv._get_synthesizerVersion = swift__get_synthesizerVersion;
	klass->epv.driverInit              = swift_driverInit;
	klass->epv.isInitialized           = swift_driver_isInitialized;
	klass->epv.getVoices               = swift_driver_getVoices;
	klass->epv.getAllVoices            = swift_driver_getAllVoices;
	klass->epv.createSpeaker           = swift_driver_createSpeaker;
}

BONOBO_TYPE_FUNC_FULL(SwiftSynthesisDriver, GNOME_Speech_SynthesisDriver,
                      bonobo_object_get_type(), swift_synthesis_driver);

SwiftSynthesisDriver *
swift_synthesis_driver_new(void)
{
	SwiftSynthesisDriver *driver;
	driver = g_object_new(SWIFT_SYNTHESIS_DRIVER_TYPE, NULL);
	return driver;
}

int
main(int argc, char **argv)
{
	SwiftSynthesisDriver *driver;
	char *obj_id;
	int ret;

	if (!bonobo_init(&argc, argv)) {
		g_error("Could not initialize Bonobo Activation / Bonobo");
	}

	/* Initialize threads */

	obj_id = "OAFIID:GNOME_Speech_SynthesisDriver_Swift:proto0.3";

	driver = swift_synthesis_driver_new();
	if (!driver) {
		g_error("Error creating speech synthesis driver object.\n");
	}

	ret = bonobo_activation_active_server_register(obj_id,
						       bonobo_object_corba_objref(bonobo_object(driver)));

	if (ret != Bonobo_ACTIVATION_REG_SUCCESS) {
		g_error ("Error registering speech synthesis driver.\n");
	} else {
		bonobo_main();
	}

	return 0;
}
