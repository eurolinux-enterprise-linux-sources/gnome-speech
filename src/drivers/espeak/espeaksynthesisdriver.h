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
 * espeaksynthesisdriver.h: Definition of the EspeakSynthesisDriver
 *                            object-- a GNOME Speech driver for eSpeak
 *                            (implementation in espeaksynthesisdriver.c)
 *
 */


#ifndef __ESPEAK_SYNTHESIS_DRIVER_H_
#define __ESPEAK_SYNTHESIS_DRIVER_H_

#include <bonobo/bonobo-object.h>
#include <glib/gthread.h>
#include <gnome-speech/gnome-speech.h>
#include <speak_lib.h>
#include "espeakspeaker.h"

#define ESPEAK_SYNTHESIS_DRIVER_TYPE        (espeak_synthesis_driver_get_type ())
#define ESPEAK_SYNTHESIS_DRIVER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ESPEAK_SYNTHESIS_DRIVER_TYPE, EspeakSynthesisDriver))
#define ESPEAK_SYNTHESIS_DRIVER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), espeak_SYNTHESIS_driver_TYPE, EspeakSynthesisDriverClass))
#define ESPEAK_SYNTHESIS_DRIVER_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), ESPEAK_SYNTHESIS_DRIVER_TYPE, EspeakSynthesisDriverClass))
#define IS_ESPEAK_SYNTHESIS_DRIVER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ESPEAK_SYNTHESIS_DRIVER_TYPE))
#define IS_ESPEAK_SYNTHESIS_DRIVER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ESPEAK_SYNTHESIS_DRIVER_TYPE))

typedef struct {
	BonoboObject parent;

	GMutex *mutex;

  //	ECIHand handle;

	EspeakSpeaker *last_speaker;
	guint timeout_id;
	guint text_idle;
	GSList *utterance_queue;
	gboolean initialized;
} EspeakSynthesisDriver;

typedef struct {
	BonoboObjectClass parent_class;

	POA_GNOME_Speech_SynthesisDriver__epv epv;
} EspeakSynthesisDriverClass;

GType
espeak_synthesis_driver_get_type   (void);

EspeakSynthesisDriver *
espeak_synthesis_driver_new (void);
gint
espeak_synthesis_driver_say (EspeakSynthesisDriver *d,
			     EspeakSpeaker *s,
			     gchar *text);
gboolean
espeak_synthesis_driver_stop (EspeakSynthesisDriver *d);
void
espeak_synthesis_driver_wait (EspeakSynthesisDriver *d);
gboolean
espeak_synthesis_driver_is_speaking (EspeakSynthesisDriver *d);
gboolean
espeak_synthesis_driver_set_param (const EspeakSynthesisDriver *d,
				   espeak_PARAMETER param,
				   gint new_value);
gboolean
espeak_synthesis_driver_set_voice_spec (const EspeakSynthesisDriver *d,
					espeak_VOICE *voice_spec);
gboolean
espeak_synthesis_driver_set_voice_name (const EspeakSynthesisDriver *d,
					gchar* name);

#endif /* __ESPEAK_SYNTHESIS_DRIVER_H_ */
