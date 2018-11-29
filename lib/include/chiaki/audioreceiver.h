/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CHIAKI_AUDIORECEIVER_H
#define CHIAKI_AUDIORECEIVER_H

#include "common.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_audio_receiver_t
{
	struct chiaki_session_t *session;
	ChiakiLog *log;
} ChiakiAudioReceiver;

CHIAKI_EXPORT void chiaki_audio_receiver_init(ChiakiAudioReceiver *audio_receiver, struct chiaki_session_t *session);
CHIAKI_EXPORT void chiaki_audio_receiver_fini(ChiakiAudioReceiver *audio_receiver);


#ifdef __cplusplus
}
#endif

#endif // CHIAKI_AUDIORECEIVER_H
