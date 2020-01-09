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
 * espeakspeaker.h: Definition of the EspeakSpeaker
 *                            object-- a GNOME Speech driver for eSpeak
 *                            (implementation in
 *                            espeaksynthesisdriver.c)
 *
 */


#ifndef __ESPEAK_SPEAKER_H_
#define __ESPEAK_SPEAKER_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <bonobo/bonobo-object.h>
#include <glib/gthread.h>
#include <speak_lib.h>
#include <gnome-speech/gnome-speech.h>



#define ESPEAK_SPEAKER_TYPE        (espeak_speaker_get_type ())
#define ESPEAK_SPEAKER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ESPEAK_SPEAKER_TYPE, EspeakSpeaker))
#define ESPEAK_SPEAKER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), espeak_SPEAKER_TYPE, EspeakSpeakerClass))
#define ESPEAK_SPEAKER_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), ESPEAK_SPEAKER_TYPE, EspeakSpeakerClass))
#define IS_ESPEAK_SPEAKER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ESPEAK_SPEAKER_TYPE))
#define IS_ESPEAK_SPEAKER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ESPEAK_SPEAKER_TYPE))

typedef struct {
	Speaker parent;
        espeak_VOICE voice;
} EspeakSpeaker;

typedef struct {
	SpeakerClass parent_class;
} EspeakSpeakerClass;

GType
espeak_speaker_get_type   (void);

EspeakSpeaker *
espeak_speaker_new (GObject *d,
		      const GNOME_Speech_VoiceInfo *voice_spec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ESPEAK_SPEAKER_H_ */
