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
 * loquendospeaker.h: Definition of the LoquendoSpeaker
 *                            object-- a GNOME Speech driver for
 *                            Loquendo TTS SDK (implementation in
 *                            loquendosynthesisdriver.c)
 *
 */


#ifndef __LOQUENDO_SPEAKER_H_
#define __LOQUENDO_SPEAKER_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <bonobo/bonobo-object.h>
#include <glib/gthread.h>
#include <loqtts.h>
#include <gnome-speech/gnome-speech.h>


#define LOQUENDO_SPEAKER_TYPE        (loquendo_speaker_get_type ())
#define LOQUENDO_SPEAKER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), LOQUENDO_SPEAKER_TYPE, LoquendoSpeaker))
#define LOQUENDO_SPEAKER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), loquendo_SPEAKER_TYPE, LoquendoSpeakerClass))
#define LOQUENDO_SPEAKER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), LOQUENDO_SPEAKER_TYPE, LoquendoSpeakerClass))
#define IS_LOQUENDO_SPEAKER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), LOQUENDO_SPEAKER_TYPE))
#define IS_LOQUENDO_SPEAKER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), LOQUENDO_SPEAKER_TYPE))


typedef struct {
	Speaker parent;
} LoquendoSpeaker;

typedef struct {
	SpeakerClass parent_class;
} LoquendoSpeakerClass;

GType loquendo_speaker_get_type   (void);
LoquendoSpeaker * loquendo_speaker_new(const GObject *driver,const GNOME_Speech_VoiceInfo *voice_spec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LOQUENDO_SPEAKER_H_ */

