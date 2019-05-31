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

#ifndef CHIAKI_SESSION_H
#define CHIAKI_SESSION_H

#include "nagare.h"
#include "common.h"
#include "thread.h"
#include "log.h"
#include "ctrl.h"
#include "rpcrypt.h"
#include "takion.h"
#include "ecdh.h"
#include "audio.h"
#include "audioreceiver.h"
#include "videoreceiver.h"

#include <stdint.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CHIAKI_RP_DID_SIZE 32
#define CHIAKI_SESSION_ID_SIZE_MAX 80
#define CHIAKI_HANDSHAKE_KEY_SIZE 0x10

typedef struct chiaki_connect_info_t
{
	const char *host; // null terminated
	const char *regist_key; // null terminated
	const char *ostype; // null terminated
	char auth[0x10]; // must be completely filled (pad with \0)
	uint8_t morning[0x10];
	uint8_t did[CHIAKI_RP_DID_SIZE];
} ChiakiConnectInfo;


typedef enum {
	CHIAKI_QUIT_REASON_NONE,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH,
	CHIAKI_QUIT_REASON_CTRL_UNKNOWN,
	CHIAKI_QUIT_REASON_CTRL_CONNECTION_REFUSED
} ChiakiQuitReason;

typedef struct chiaki_quit_event_t
{
	ChiakiQuitReason reason;
} ChiakiQuitEvent;

typedef struct chiaki_audio_stream_info_event_t
{
	ChiakiAudioHeader audio_header;
} ChiakiAudioStreamInfoEvent;


typedef enum {
	CHIAKI_EVENT_QUIT,
	CHIAKI_EVENT_AUDIO_STREAM_INFO
} ChiakiEventType;

typedef struct chiaki_event_t
{
	ChiakiEventType type;
	union
	{
		ChiakiQuitEvent quit;
		ChiakiAudioStreamInfoEvent audio_stream_info;
	};
} ChiakiEvent;

typedef void (*ChiakiEventCallback)(ChiakiEvent *event, void *user);
typedef void (*ChiakiAudioFrameCallback)(int16_t *buf, size_t samples_count, void *user);



typedef struct chiaki_session_t
{
	struct
	{
		struct addrinfo *host_addrinfos;
		struct addrinfo *host_addrinfo_selected;
		char hostname[128];
		char *regist_key;
		char *ostype;
		char auth[CHIAKI_KEY_BYTES];
		uint8_t morning[CHIAKI_KEY_BYTES];
		uint8_t did[CHIAKI_RP_DID_SIZE];
	} connect_info;

	uint8_t nonce[CHIAKI_KEY_BYTES];
	ChiakiRPCrypt rpcrypt;
	char session_id[CHIAKI_SESSION_ID_SIZE_MAX]; // zero-terminated
	uint8_t handshake_key[CHIAKI_HANDSHAKE_KEY_SIZE];
	unsigned int mtu;
	unsigned int rtt;
	ChiakiECDH ecdh;

	ChiakiQuitReason quit_reason;

	ChiakiEventCallback event_cb;
	void *event_cb_user;
	ChiakiAudioFrameCallback audio_frame_cb;
	void *audio_frame_cb_user;

	ChiakiThread session_thread;

	ChiakiCtrl ctrl;
	ChiakiCond ctrl_cond;
	ChiakiMutex ctrl_cond_mutex;
	bool ctrl_session_id_received;

	ChiakiLog log;

	ChiakiNagare nagare;
	ChiakiAudioReceiver *audio_receiver;
	ChiakiVideoReceiver *video_receiver;
} ChiakiSession;

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info);
CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_start(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_join(ChiakiSession *session);

static inline void chiaki_session_set_event_cb(ChiakiSession *session, ChiakiEventCallback cb, void *user)
{
	session->event_cb = cb;
	session->event_cb_user = user;
}

static inline void chiaki_session_set_audio_frame_cb(ChiakiSession *session, ChiakiAudioFrameCallback cb, void *user)
{
	session->audio_frame_cb = cb;
	session->audio_frame_cb_user = user;
}

static inline void chiaki_session_set_quit_reason(ChiakiSession *session, ChiakiQuitReason reason)
{
	if(session->quit_reason != CHIAKI_QUIT_REASON_NONE)
		return;
	session->quit_reason = reason;
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_SESSION_H
