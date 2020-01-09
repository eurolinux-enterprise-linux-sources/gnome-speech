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
 * viavoicesynthesisdriver.c: Implements the ViavoiceSynthesisDriver object--
 *                            a GNOME Speech driver for IBM's Viavoice TTS RTK
 *
 */

#include <libbonobo.h>
#include <glib/gmain.h>
#include <gnome-speech/gnome-speech.h>
#include <string.h>
#include "viavoicesynthesisdriver.h"
#include "viavoicespeaker.h"
 

#define VERSION_LENGTH 20

typedef struct {
	GNOME_Speech_SpeechCallback cb;
	GNOME_Speech_speech_callback_type type;
	gint text_id;
	gint offset;
} index_queue_entry;



static gint text_id = 0;
static GObjectClass *parent_class;

typedef struct _eciLocale {
	char *pLocale;
	enum ECILanguageDialect langID;
	char* charset;
} eciLocale, *eciLocaleList;

static eciLocale eciLocales[] = {
	{"en_US", eciGeneralAmericanEnglish, "ISO-8859-1"},
	{"en_GB", eciBritishEnglish, "ISO-8859-1"},
	{"es_ES", eciCastilianSpanish, "ISO-8859-1"},
	{"es_MX", eciMexicanSpanish, "ISO-8859-1"},
	{"fr_FR", eciStandardFrench, "ISO-8859-1"},
	{"ca_FR", eciCanadianFrench, "ISO-8859-1"},
	{"de_DE", eciStandardGerman, "ISO-8859-1"},
	{"it_IT", eciStandardItalian, "ISO-8859-1"},
	{"zh_CN", eciMandarinChinese, "GBK"},
	{"zh_CN_GB", eciMandarinChineseGB, "GBK"},
	{"zh_CN_PinYin", eciMandarinChinesePinYin, "GBK"},
	{"zh_CN_UCS", eciMandarinChineseUCS, "UCS2"},
	{"zh_TW", eciTaiwaneseMandarin, "BIG5"},
	{"zh_TW_Big5", eciTaiwaneseMandarinBig5, "BIG5"},
	{"zh_TW_ZhuYin", eciTaiwaneseMandarinZhuYin, "BIG5"},
	{"zh_TW_PinYin", eciTaiwaneseMandarinPinYin, "BIG5"},
	{"zh_TW_UCS", eciTaiwaneseMandarinUCS, "UCS2"},
	{"pt_BR", eciBrazilianPortuguese, "ISO-8859-1"},
	{"ja_JP", eciStandardJapanese, "SJIS"},
	{"ja_JP_SJIS", eciStandardJapaneseSJIS, "SJIS"},
	{"ja_JP_UCS", eciStandardJapaneseUCS, "UCS2"},
	{"fi_FI", eciStandardFinnish, "ISO-8859-1"},
	{"ko_KR", eciStandardKorean, "UHC"},
	{"ko_KR_UHC", eciStandardKoreanUHC, "UHC"},
	{"ko_KR_UCS", eciStandardKoreanUCS, "UCS2"},
	{"zh_HK", eciStandardCantonese, "GBK"},
	{"zh_HK_GB", eciStandardCantoneseGB, "GBK"},
	{"zh_HK_UCS", eciStandardCantoneseUCS, "UCS2"},
	{"zh_HK", eciHongKongCantonese, "BIG5"},
	{"zh_HK_BIG5", eciHongKongCantoneseBig5, "BIG5"},
	{"zh_HK_UCS", eciHongKongCantoneseUCS, "UCS-2"},
	{"nl_BE", eciStandardDutch, "ISO-8859-1"},
	{"no_NO", eciStandardNorwegian, "ISO-8859-1"},
	{"sv_SE", eciStandardSwedish, "ISO-8859-1"},
	{"da_DK", eciStandardDanish, "ISO-8859-1"},
	{"en_US", eciStandardReserved, "ISO-8859-1"},
	{"th_TH", eciStandardThai, "TIS-620"},
	{"th_TH_TIS", eciStandardThaiTIS, "TIS-620"},
	{NULL, 0, NULL}
};

/*
 *
 * index_queue_entry functions
 *
 */

static void
index_queue_entry_destroy (index_queue_entry *e)
{
	CORBA_Environment ev;
	g_return_if_fail (e);
	CORBA_exception_init (&ev);
	CORBA_Object_release (e->cb, &ev);
	CORBA_exception_free (&ev);
	g_free (e);
}


static void
viavoice_synthesis_driver_flush_index_queue (ViavoiceSynthesisDriver *d)
{
	GSList *tmp;
	
	/* Flush the index queues */

	g_mutex_lock (d->mutex);
	for (tmp = d->index_queue; tmp; tmp = tmp->next) {
		index_queue_entry *e = (index_queue_entry *) tmp->data;
		index_queue_entry_destroy (e);
	}
	g_slist_free (d->index_queue);
	d->index_queue = NULL;
	g_mutex_unlock (d->mutex);
}

static ViavoiceSynthesisDriver *
viavoice_synthesis_driver_from_servant (PortableServer_Servant *servant)
{
	return VIAVOICE_SYNTHESIS_DRIVER(bonobo_object_from_servant (servant));
}


static gboolean
viavoice_synthesis_driver_timeout_callback (void *data)
{
	ViavoiceSynthesisDriver *d = VIAVOICE_SYNTHESIS_DRIVER (data);

	eciSpeaking (d->handle);
	return TRUE;
}



static enum ECICallbackReturn
viavoice_synthesis_driver_index_callback (ECIHand handle,
			   enum ECIMessage msg,
			   long param,
			   void *data)
{
	ViavoiceSynthesisDriver *d = VIAVOICE_SYNTHESIS_DRIVER(data);
	index_queue_entry *e = NULL;
	CORBA_Environment ev;
	
	g_mutex_lock (d->mutex);
	if (d && d->index_queue) {
		e = (index_queue_entry *) d->index_queue->data;
		d->index_queue = g_slist_remove_link (d->index_queue, d->index_queue);
	}
	g_mutex_unlock (d->mutex);
	switch (msg) {
	case eciIndexReply:
		if (e) {
			CORBA_exception_init (&ev);
			GNOME_Speech_SpeechCallback_notify (e->cb,
							    e->type,
							    e->text_id,
							    e->offset,
							    &ev);
			CORBA_Object_release (e->cb, &ev);
			CORBA_exception_free (&ev);
			g_free (e);
			e = NULL;
		}
		
		break;
	case eciWaveformBuffer:
	case eciPhonemeBuffer:
	case eciPhonemeIndexReply:
		break;
	}
	if (e)
		g_free (e);
	return eciDataProcessed;
}



static CORBA_string
viavoice__get_driverName (PortableServer_Servant servant,
			  CORBA_Environment * ev)
{
	return CORBA_string_dup ("IBM Viavoice GNOME Speech Driver");
}




static CORBA_string
viavoice__get_synthesizerName (PortableServer_Servant servant,
			       CORBA_Environment * ev)
{
	return CORBA_string_dup ("IBM Viavoice");
}



static CORBA_string
viavoice__get_driverVersion (PortableServer_Servant aservant,
			     CORBA_Environment * ev)
{
	return CORBA_string_dup ("0.3");
}



static CORBA_string
viavoice__get_synthesizerVersion (PortableServer_Servant
				  servant,
				  CORBA_Environment * ev)
{
	char version[VERSION_LENGTH];
	
	eciVersion (version);

	return CORBA_string_dup (version);
}


static CORBA_boolean
viavoice_driverInit (PortableServer_Servant servant,
		     CORBA_Environment *ev)
{
	ViavoiceSynthesisDriver *d = viavoice_synthesis_driver_from_servant (servant);

	if (!d->initialized) {

		/* Create a handle and initialize it */

		d->handle = eciNew ();

		if (d->handle == NULL_ECI_HAND)
			return FALSE;
		
		/* Set sample rate */

		eciSetVoiceParam (d->handle, 0, eciSampleRate, 1);

		/* Turn on manual synthesis mode */

		eciSetParam (d->handle, eciSynthMode, 1);

		/* Turn off abbbreviation processing */

		eciSetParam (d->handle, eciDictionary, 1);
				
		/* Add a timeout callback to poke this instance of Viavoice */

		d->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH_IDLE, 100,
						    viavoice_synthesis_driver_timeout_callback, d, NULL);
		
		/* Set up ECI callback */

		eciRegisterCallback (d->handle,
				     viavoice_synthesis_driver_index_callback,
				     d);
		
		d->initialized = TRUE;
		}
	return d->initialized;
}


static CORBA_boolean
viavoice_isInitialized (PortableServer_Servant servant,
			CORBA_Environment *ev)
{
	ViavoiceSynthesisDriver *driver = viavoice_synthesis_driver_from_servant (servant);
	return driver->initialized;
}


static GSList *
get_voice_list (const GNOME_Speech_VoiceInfo *vs)
{
	GSList *l = NULL;
	ECIHand tmp_handle;
	int h, i;
	enum ECILanguageDialect origLang;
  
	tmp_handle = eciNew ();
	if (tmp_handle == NULL_ECI_HAND)
		return NULL;

	origLang = eciGetParam(tmp_handle, eciLanguageDialect);

	for (h = 0; NULL != eciLocales[h].pLocale; h++) {

		if (vs && (0 != strcmp(vs->language, eciLocales[h].pLocale))) {
			continue;
		}

		if (eciSetParam(tmp_handle, eciLanguageDialect,
		eciLocales[h].langID) < 0) {
			continue;
		}

		gboolean is_UCS_2 = (0==strcmp(eciLocales[h].charset, "UCS2"));

		for (i = 1; i <= ECI_PRESET_VOICES; i++) {
			GNOME_Speech_VoiceInfo *info;
			char tmp[MAX_VOICE_LENGTH];			
			int gender;
			info = GNOME_Speech_VoiceInfo__alloc ();
			eciGetVoiceName (tmp_handle, i, tmp);

			if (is_UCS_2)
			  { // UCS-2 to ASCII conversion
			    // Using IBM TTS 6.7.2.4 on Linux, 
			    // the voices names can be converted to ASCII.
			    int j=0;
			    for (j = 1; 2*j <= MAX_VOICE_LENGTH-1; j++) {
			      tmp[j] = tmp[2*j];
			    }
			  }

			info->language= CORBA_string_dup(eciLocales[h].pLocale);
			info->name = CORBA_string_dup (tmp);
			gender = eciGetVoiceParam (tmp_handle, i, eciGender);
			if (gender == 0)
				info->gender = GNOME_Speech_gender_male;
			else if (gender == 1)
				info->gender = GNOME_Speech_gender_female;
			l = g_slist_prepend (l, info);
		}
	}
	l = g_slist_reverse (l);
	eciSetParam(tmp_handle, eciLanguageDialect, origLang);
	eciDelete (tmp_handle);
	return l;
}



static void
voice_list_free (GSList *l)
{
	GSList *tmp = l;

	while (tmp) {
		CORBA_free (tmp->data);
		tmp = tmp->next;
	}
	g_slist_free (l);
}



static GNOME_Speech_VoiceInfoList *
voice_info_list_from_voice_list (GSList *l)
{
	int i = 0;
	GNOME_Speech_VoiceInfoList *rv = GNOME_Speech_VoiceInfoList__alloc ();
  
	if (!l)
	{
	        rv->_length = 0;
		return rv;
	}

	rv->_length = rv->_maximum = g_slist_length (l);
	rv->_buffer = GNOME_Speech_VoiceInfoList_allocbuf (rv->_length);

	while (l) {
		GNOME_Speech_VoiceInfo *info =
			(GNOME_Speech_VoiceInfo *) l->data;
		rv->_buffer[i].name = CORBA_string_dup (info->name);
		rv->_buffer[i].gender = info->gender;
		rv->_buffer[i].language = CORBA_string_dup(info->language);
		i++;
		l = l->next;
	}
	return rv;
}



static GNOME_Speech_VoiceInfoList *
viavoice_getVoices (PortableServer_Servant servant,
		    const GNOME_Speech_VoiceInfo *voice_spec,
		    CORBA_Environment *ev)
{
	GNOME_Speech_VoiceInfoList *rv;
	GSList *l;

	l = get_voice_list (voice_spec);
	rv = voice_info_list_from_voice_list (l);
	voice_list_free (l);
	return rv;
}



static GNOME_Speech_VoiceInfoList *
viavoice_getAllVoices (PortableServer_Servant servant,
		       CORBA_Environment *ev)
{
	GNOME_Speech_VoiceInfoList *rv;
	GSList *l;

	l = get_voice_list (NULL);
	rv = voice_info_list_from_voice_list (l);
	voice_list_free (l);
	return rv;
}



static GNOME_Speech_Speaker
viavoice_createSpeaker (PortableServer_Servant servant,
			const GNOME_Speech_VoiceInfo *voice_spec,
			CORBA_Environment *ev)
{
	ViavoiceSynthesisDriver *d = viavoice_synthesis_driver_from_servant (servant);
	ViavoiceSpeaker *s = viavoice_speaker_new (G_OBJECT(d), voice_spec);
	return CORBA_Object_duplicate(bonobo_object_corba_objref(BONOBO_OBJECT(s)), ev);
}



static void
viavoice_synthesis_driver_init (ViavoiceSynthesisDriver *driver)
{
	driver->mutex = g_mutex_new ();
	driver->last_speaker = NULL;
	driver->index_queue = NULL;
	driver->timeout_id = -1;
}



static void
viavoice_synthesis_driver_finalize (GObject *obj)
{
	ViavoiceSynthesisDriver *d = VIAVOICE_SYNTHESIS_DRIVER (obj);

        /* Destroy the ECI instance */

	if (d->handle != NULL_ECI_HAND) {
		eciSynchronize (d->handle);
		eciDelete (d->handle);
	}

	/* Remove timeout */

	if (d->timeout_id >= 0)
		g_source_remove (d->timeout_id);

	/* Flush the index queue */

	viavoice_synthesis_driver_flush_index_queue (d);

	if (parent_class->finalize)
		parent_class->finalize (obj);
	bonobo_main_quit ();
}



static void
viavoice_synthesis_driver_class_init (ViavoiceSynthesisDriverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
  
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = viavoice_synthesis_driver_finalize;

	/* Initialize parent class epv table */

	klass->epv._get_driverName = viavoice__get_driverName;
	klass->epv._get_driverVersion = viavoice__get_driverVersion;
	klass->epv._get_synthesizerName = viavoice__get_synthesizerName;
	klass->epv._get_synthesizerVersion = viavoice__get_synthesizerVersion;
	klass->epv.getVoices = viavoice_getVoices;
	klass->epv.getAllVoices = viavoice_getAllVoices;
	klass->epv.createSpeaker = viavoice_createSpeaker;  
	klass->epv.driverInit = viavoice_driverInit;
	klass->epv.isInitialized = viavoice_isInitialized;
}




BONOBO_TYPE_FUNC_FULL (ViavoiceSynthesisDriver,
		       GNOME_Speech_SynthesisDriver,
		       bonobo_object_get_type (),
		       viavoice_synthesis_driver);



ViavoiceSynthesisDriver *
viavoice_synthesis_driver_new (void)
{
	ViavoiceSynthesisDriver *driver;
	driver = g_object_new (VIAVOICE_SYNTHESIS_DRIVER_TYPE, NULL);
	return driver;
}




int
main (int argc,
      char **argv)
{
	ViavoiceSynthesisDriver *driver;
	char *obj_id;
	int ret;
  
	if (!bonobo_init (&argc, argv)) {
		g_error ("Could not initialize Bonobo Activation / Bonobo");
	}

	/* If threads haven't been initialized, initialize them */

	if (!g_threads_got_initialized)
		g_thread_init (NULL);
	
	obj_id = "OAFIID:GNOME_Speech_SynthesisDriver_Viavoice:proto0.3";

	driver = viavoice_synthesis_driver_new ();
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


static void
viavoice_synthesis_driver_add_index (ViavoiceSynthesisDriver *d,
				     GNOME_Speech_SpeechCallback cb,
				     GNOME_Speech_speech_callback_type type,
				     gint text_id,
				     gint offset)
{
	index_queue_entry *e;
	CORBA_Environment ev;
	
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

	eciInsertIndex (d->handle, 0);
}



gint
viavoice_synthesis_driver_say (ViavoiceSynthesisDriver *d,
			       ViavoiceSpeaker *s,
			       gchar *text)
{
	Speaker *speaker = SPEAKER (s);
	gchar *end, *word, *converted_text;
        GError *error=NULL;
	int h;
	gint status=-1;

	/* look for the expected charset */
	int lang = eciGetParam(d->handle, eciLanguageDialect);
	if (-1 == lang) {
	    return -1;
	  }

	for (h = 0; NULL != eciLocales[h].pLocale; h++) {	  
	  if (eciLocales[h].langID == lang) {	    
	    break;
	  }
	}

	if (NULL == eciLocales[h].pLocale) {
	  h = 0;
	}

	converted_text=g_convert_with_fallback(text, -1, eciLocales[h].charset, "UTF-8", " ", NULL, NULL, &error);
	gchar *beginning = converted_text;

        /* If this speaker wasn't the last one to speak, reset the speech parameters */

	if (d->last_speaker != s || speaker_needs_parameter_refresh(speaker)) {
		speaker_refresh_parameters (speaker);
		d->last_speaker = s;
	}
	
	text_id++;
	if (converted_text != NULL) {
	  if (speaker->cb != CORBA_OBJECT_NIL) {
		viavoice_synthesis_driver_add_index(d,
						    speaker->cb,
						    GNOME_Speech_speech_callback_speech_started,
						    text_id, 0);
		
		/* Add index between words */
		
		while (*beginning) {
			end = beginning;
			while (*end && !g_ascii_isspace(*end))
				end++;
			word = g_strndup (beginning, end-beginning);
			eciAddText (d->handle, word);
			g_free (word);
			viavoice_synthesis_driver_add_index(d,
						   speaker->cb,
						   GNOME_Speech_speech_callback_speech_progress,
						   text_id, end-text);
			if (!*end)
				break;
			beginning = end+1;
		}
		viavoice_synthesis_driver_add_index(d,
						   speaker->cb,
						   GNOME_Speech_speech_callback_speech_ended,
						   text_id, 0);
	}
	else {
		eciAddText (d->handle, (char *) converted_text);
	}

	g_free (converted_text);

	if (eciSynthesize (d->handle))
		status = text_id;
      }

      return status;
}


gboolean
viavoice_synthesis_driver_stop (ViavoiceSynthesisDriver *d)
{
	viavoice_synthesis_driver_flush_index_queue (d);
	eciStop (d->handle);
	return TRUE;
}


void
viavoice_synthesis_driver_wait (ViavoiceSynthesisDriver *d)
{
	eciSynchronize (d->handle);
}


gboolean
viavoice_synthesis_driver_is_speaking (ViavoiceSynthesisDriver *d)
{
	return eciSpeaking (d->handle);
}


void
viavoice_synthesis_driver_set_voice_param (const ViavoiceSynthesisDriver *d,
					   enum ECIVoiceParam param,
					   gint new_value)
{
	eciSetVoiceParam (d->handle, 0, param, new_value);
}

void
viavoice_synthesis_driver_set_device_param (const ViavoiceSynthesisDriver *d,
					   enum ECIParam param,
					   gint new_value)
{
	eciSetParam (d->handle, param, new_value);
}


gboolean
viavoice_synthesis_driver_set_voice (const ViavoiceSynthesisDriver *d,
				     gint v)
{
	return eciCopyVoice (d->handle,
		      (int) v,
		      0);
}

char *
dialect_to_language(enum ECILanguageDialect langID) {
	int h;

	for (h = 0; NULL != eciLocales[h].pLocale; h++) {

		if (langID == eciLocales[h].langID) {
			return  eciLocales[h].pLocale;
		}
	}
	return NULL;
}

enum ECILanguageDialect
viavoice_synthesis_driver_language_to_dialect(char *lang) {
	int h;

	for (h = 0; NULL != eciLocales[h].pLocale; h++) {

		if (0 == strcmp(lang, eciLocales[h].pLocale)) {
			return eciLocales[h].langID;
		}
	}

	return eciGeneralAmericanEnglish;
}
