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
 * swiftsynthesisdriver.h: Definition of the SwiftSynthesisDriver
 *                           object-- a GNOME Speech driver for Cepstral's
 *                           Swift TTS engine (implementation in
 *                           swiftsynthesisdriver.c)
 */

#ifndef __SWIFT_SYNTHESIS_DRIVER_H_
#define __SWIFT_SYNTHESIS_DRIVER_H_

#include <bonobo/bonobo-object.h>
#include <gnome-speech/gnome-speech.h>

#define SWIFT_SYNTHESIS_DRIVER_TYPE        (swift_synthesis_driver_get_type ())
#define SWIFT_SYNTHESIS_DRIVER(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), SWIFT_SYNTHESIS_DRIVER_TYPE, SwiftSynthesisDriver))
#define SWIFT_SYNTHESIS_DRIVER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), swift_SYNTHESIS_driver_TYPE, SwiftSynthesisDriverClass))
#define SWIFT_SYNTHESIS_DRIVER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), SWIFT_SYNTHESIS_DRIVER_TYPE, SwiftSynthesisDriverClass))
#define IS_SWIFT_SYNTHESIS_DRIVER(o)       (G_TYPE_CHECK_INSTANCE_TYPE((o), SWIFT_SYNTHESIS_DRIVER_TYPE))
#define IS_SWIFT_SYNTHESIS_DRIVER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), SWIFT_SYNTHESIS_DRIVER_TYPE))

typedef struct {
	BonoboObject parent;
	gboolean     initialized;
        swift_engine *tts_engine;
        swift_port   *tts_port;
	gboolean     speaking;
	GSList       *utterances;
	guint        timeout_id;
	int          status_pipe[2];
} SwiftSynthesisDriver;

typedef struct {
	BonoboObjectClass parent_class;
	POA_GNOME_Speech_SynthesisDriver__epv epv;
} SwiftSynthesisDriverClass;

#include "swiftspeaker.h"

swift_result_t 
swift_event_callback (swift_event *event,
		      swift_event_t type,
		      void *udata);

GType
swift_synthesis_driver_get_type (void);

SwiftSynthesisDriver *
swift_synthesis_driver_new (void);

gint
swift_synthesis_driver_say (SwiftSynthesisDriver *d,
			    SwiftSpeaker *s,
			    const gchar *text);

gboolean
swift_synthesis_driver_stop (SwiftSynthesisDriver *d);

gboolean
swift_synthesis_driver_is_speaking (SwiftSynthesisDriver *d);

#endif /* __SWIFT_SYNTHESIS_DRIVER_H_ */
