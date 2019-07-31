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
#include "audio.h"
#include "takion.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct chiaki_audio_receiver_t
{
	struct chiaki_session_t *session;
	ChiakiLog *log;
	ChiakiMutex mutex;
	struct OpusDecoder *opus_decoder;
	ChiakiAudioHeader audio_header;
	ChiakiSeqNum16 frame_index_prev;
	bool frame_index_startup; // whether frame_index_prev has definitely not wrapped yet
} ChiakiAudioReceiver;

CHIAKI_EXPORT ChiakiErrorCode chiaki_audio_receiver_init(ChiakiAudioReceiver *audio_receiver, struct chiaki_session_t *session);
CHIAKI_EXPORT void chiaki_audio_receiver_fini(ChiakiAudioReceiver *audio_receiver);
CHIAKI_EXPORT void chiaki_audio_receiver_stream_info(ChiakiAudioReceiver *audio_receiver, ChiakiAudioHeader *audio_header);
CHIAKI_EXPORT void chiaki_audio_receiver_av_packet(ChiakiAudioReceiver *audio_receiver, ChiakiTakionAVPacket *packet);

static inline ChiakiAudioReceiver *chiaki_audio_receiver_new(struct chiaki_session_t *session)
{
	ChiakiAudioReceiver *audio_receiver = CHIAKI_NEW(ChiakiAudioReceiver);
	if(!audio_receiver)
		return NULL;
	ChiakiErrorCode err = chiaki_audio_receiver_init(audio_receiver, session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		free(audio_receiver);
		return NULL;
	}
	return audio_receiver;
}

static inline void chiaki_audio_receiver_free(ChiakiAudioReceiver *audio_receiver)
{
	if(!audio_receiver)
		return;
	chiaki_audio_receiver_fini(audio_receiver);
	free(audio_receiver);
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_AUDIORECEIVER_H
