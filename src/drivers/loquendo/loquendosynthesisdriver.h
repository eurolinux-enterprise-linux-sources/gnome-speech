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
 * loquendosynthesisdriver.h: Definition of  the LoquendoSynthesisDriver
 *                            object-- a GNOME Speech driver for 
 *                            Loquendo TTS (implementation in
 *                            loquendosynthesisdriver.c)
 *
 */


#ifndef __LOQUENDO_SYNTHESIS_DRIVER_H_
#define __LOQUENDO_SYNTHESIS_DRIVER_H_


#include <bonobo/bonobo-object.h>
#include <glib/gthread.h>
#include <gnome-speech/gnome-speech.h>
#include "loquendospeaker.h"

#define LOQUENDO_SYNTHESIS_DRIVER_TYPE        (loquendo_synthesis_driver_get_type ())
#define LOQUENDO_SYNTHESIS_DRIVER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), LOQUENDO_SYNTHESIS_DRIVER_TYPE, LoquendoSynthesisDriver))
#define LOQUENDO_SYNTHESIS_DRIVER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), loquendo_SYNTHESIS_driver_TYPE, LoquendoSynthesisDriverClass))
#define LOQUENDO_SYNTHESIS_DRIVER_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), LOQUENDO_SYNTHESIS_DRIVER_TYPE, LoquendoSynthesisDriverClass))
#define IS_LOQUENDO_SYNTHESIS_DRIVER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), LOQUENDO_SYNTHESIS_DRIVER_TYPE))
#define IS_LOQUENDO_SYNTHESIS_DRIVER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), LOQUENDO_SYNTHESIS_DRIVER_TYPE))

typedef struct {
	BonoboObject parent;

	GMutex *mutex;

	ttsHandleType handle;

	LoquendoSpeaker *last_speaker;
	guint timeout_id;
	GSList *index_queue;
	gboolean initialized;

	gchar *cadena;

} LoquendoSynthesisDriver;

typedef struct {
	BonoboObjectClass parent_class;

	POA_GNOME_Speech_SynthesisDriver__epv epv;
} LoquendoSynthesisDriverClass;


GType loquendo_synthesis_driver_get_type   (void);
LoquendoSynthesisDriver * loquendo_synthesis_driver_new (void);
gint loquendo_synthesis_driver_say (LoquendoSynthesisDriver *d,LoquendoSpeaker *s,gchar *text);
gboolean loquendo_synthesis_driver_stop (LoquendoSynthesisDriver *d);
void loquendo_synthesis_driver_wait (LoquendoSynthesisDriver *d);
gboolean loquendo_synthesis_driver_is_speaking (LoquendoSynthesisDriver *d);
void loquendo_synthesis_driver_set_voice_param (const LoquendoSynthesisDriver *d,const char* param,const char* new_value);
void loquendo_synthesis_driver_set_device_param (const LoquendoSynthesisDriver *d,const char* param,const char* new_value);
gboolean loquendo_synthesis_driver_set_voice (const LoquendoSynthesisDriver *d,gint v);

					   
#endif /* __LOQUENDO_SYNTHESIS_DRIVER_H_ */

