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

#ifndef CHIAKI_VIDEORECEIVER_H
#define CHIAKI_VIDEORECEIVER_H

#include "common.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_video_receiver_t
{
	struct chiaki_session_t *session;
	ChiakiLog *log;

} ChiakiVideoReceiver;

CHIAKI_EXPORT void chiaki_video_receiver_init(ChiakiVideoReceiver *video_receiver, struct chiaki_session_t *session);
CHIAKI_EXPORT void chiaki_video_receiver_fini(ChiakiVideoReceiver *video_receiver);

static inline ChiakiVideoReceiver *chiaki_video_receiver_new(struct chiaki_session_t *session)
{
	ChiakiVideoReceiver *video_receiver = CHIAKI_NEW(ChiakiVideoReceiver);
	if(!video_receiver)
		return NULL;
	chiaki_video_receiver_init(video_receiver, session);
	return video_receiver;
}

static inline void chiaki_video_receiver_free(ChiakiVideoReceiver *video_receiver)
{
	if(!video_receiver)
		return;
	chiaki_video_receiver_fini(video_receiver);
	free(video_receiver);
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_VIDEORECEIVER_H
