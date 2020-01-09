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
 * swiftspeaker.h: Definition of the SwiftSpeaker
 *                            object-- a GNOME Speech driver for Cepstral's
 *                            Swift TTS Engine  (implementation in
 *                            swiftspeaker.c)
 */

#ifndef __SWIFT_SPEAKER_H_
#define __SWIFT_SPEAKER_H_

#include <bonobo/bonobo-object.h>
#include <glib/gthread.h>
#include <gnome-speech/gnome-speech.h>
#include <swift.h>

#define SWIFT_SPEAKER_TYPE        (swift_speaker_get_type())
#define SWIFT_SPEAKER(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), SWIFT_SPEAKER_TYPE, SwiftSpeaker))
#define SWIFT_SPEAKER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), swift_SPEAKER_TYPE, SwiftSpeakerClass))
#define SWIFT_SPEAKER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), SWIFT_SPEAKER_TYPE, SwiftSpeakerClass))
#define IS_SWIFT_SPEAKER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), SWIFT_SPEAKER_TYPE))
#define IS_SWIFT_SPEAKER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SWIFT_SPEAKER_TYPE))

struct voiceinfo {
        int dummy;
};

typedef struct  {
	Speaker      parent;
	swift_voice  *tts_voice;
        swift_params *tts_params;
} SwiftSpeaker;

typedef struct  {
	SpeakerClass parent_class;
} SwiftSpeakerClass;

GType
swift_speaker_get_type (void);

SwiftSpeaker *
swift_speaker_new (const GObject *driver,
		   const GNOME_Speech_VoiceInfo *voice_speec);

#endif /* __SWIFT_SPEAKER_H_ */
