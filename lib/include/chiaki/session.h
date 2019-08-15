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

#include "streamconnection.h"
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
#include "controller.h"
#include "stoppipe.h"

#include <stdint.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CHIAKI_RP_DID_SIZE 32
#define CHIAKI_SESSION_ID_SIZE_MAX 80
#define CHIAKI_HANDSHAKE_KEY_SIZE 0x10

typedef struct chiaki_connect_video_profile_t
{
	unsigned int width;
	unsigned int height;
	unsigned int max_fps;
} ChiakiConnectVideoProfile;

typedef enum {
	CHIAKI_VIDEO_RESOLUTION_PRESET_360p,
	CHIAKI_VIDEO_RESOLUTION_PRESET_540p,
	CHIAKI_VIDEO_RESOLUTION_PRESET_720p,
	CHIAKI_VIDEO_RESOLUTION_PRESET_1080p
} ChiakiVideoResolutionPreset;

typedef enum {
	CHIAKI_VIDEO_FPS_PRESET_30,
	CHIAKI_VIDEO_FPS_PRESET_60
} ChiakiVideoFPSPreset;

CHIAKI_EXPORT void chiaki_connect_video_profile_preset(ChiakiConnectVideoProfile *profile, ChiakiVideoResolutionPreset resolution, ChiakiVideoFPSPreset fps);

#define CHIAKI_SESSION_AUTH_SIZE 0x10

typedef struct chiaki_connect_info_t
{
	const char *host; // null terminated
	char regist_key[CHIAKI_SESSION_AUTH_SIZE]; // must be completely filled (pad with \0)
	uint8_t morning[0x10];
	ChiakiConnectVideoProfile video_profile;
} ChiakiConnectInfo;


typedef enum {
	CHIAKI_QUIT_REASON_NONE,
	CHIAKI_QUIT_REASON_STOPPED,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE,
	CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH,
	CHIAKI_QUIT_REASON_CTRL_UNKNOWN,
	CHIAKI_QUIT_REASON_CTRL_CONNECTION_REFUSED,
	CHIAKI_QUIT_REASON_STREAM_CONNECTION_UNKNOWN,
	CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_DISCONNECTED
} ChiakiQuitReason;

CHIAKI_EXPORT const char *chiaki_quit_reason_string(ChiakiQuitReason reason);

typedef struct chiaki_quit_event_t
{
	ChiakiQuitReason reason;
	const char *reason_str;
} ChiakiQuitEvent;

typedef struct chiaki_audio_stream_info_event_t
{
	ChiakiAudioHeader audio_header;
} ChiakiAudioStreamInfoEvent;


typedef enum {
	CHIAKI_EVENT_QUIT
} ChiakiEventType;

typedef struct chiaki_event_t
{
	ChiakiEventType type;
	union
	{
		ChiakiQuitEvent quit;
	};
} ChiakiEvent;

typedef void (*ChiakiEventCallback)(ChiakiEvent *event, void *user);
typedef void (*ChiakiAudioSettingsCallback)(uint32_t channels, uint32_t rate, void *user);
typedef void (*ChiakiAudioFrameCallback)(int16_t *buf, size_t samples_count, void *user);

/**
 * buf will always have an allocated padding of at least CHIAKI_VIDEO_BUFFER_PADDING_SIZE after buf_size
 */
typedef void (*ChiakiVideoSampleCallback)(uint8_t *buf, size_t buf_size, void *user);



typedef struct chiaki_session_t
{
	struct
	{
		struct addrinfo *host_addrinfos;
		struct addrinfo *host_addrinfo_selected;
		char hostname[128];
		char regist_key[CHIAKI_RPCRYPT_KEY_SIZE];
		uint8_t morning[CHIAKI_RPCRYPT_KEY_SIZE];
		uint8_t did[CHIAKI_RP_DID_SIZE];
		ChiakiConnectVideoProfile video_profile;
	} connect_info;

	uint8_t nonce[CHIAKI_RPCRYPT_KEY_SIZE];
	ChiakiRPCrypt rpcrypt;
	char session_id[CHIAKI_SESSION_ID_SIZE_MAX]; // zero-terminated
	uint8_t handshake_key[CHIAKI_HANDSHAKE_KEY_SIZE];
	uint32_t mtu_in;
	uint32_t mtu_out;
	uint64_t rtt_us;
	ChiakiECDH ecdh;

	ChiakiQuitReason quit_reason;
	char *quit_reason_str; // additional reason string from remote

	ChiakiEventCallback event_cb;
	void *event_cb_user;
	ChiakiAudioSettingsCallback audio_settings_cb;
	ChiakiAudioFrameCallback audio_frame_cb;
	void *audio_cb_user;
	ChiakiVideoSampleCallback  video_sample_cb;
	void *video_sample_cb_user;

	ChiakiThread session_thread;

	ChiakiCond state_cond;
	ChiakiMutex state_mutex;
	ChiakiStopPipe stop_pipe;
	bool should_stop;
	bool ctrl_failed;
	bool ctrl_session_id_received;

	ChiakiCtrl ctrl;

	ChiakiLog *log;

	ChiakiStreamConnection stream_connection;
	ChiakiAudioReceiver *audio_receiver;
	ChiakiVideoReceiver *video_receiver;

	ChiakiControllerState controller_state;
} ChiakiSession;

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info, ChiakiLog *log);
CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_start(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_stop(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_join(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_set_controller_state(ChiakiSession *session, ChiakiControllerState *state);

static inline void chiaki_session_set_event_cb(ChiakiSession *session, ChiakiEventCallback cb, void *user)
{
	session->event_cb = cb;
	session->event_cb_user = user;
}

static inline void chiaki_session_set_audio_cb(ChiakiSession *session, ChiakiAudioSettingsCallback settings_cb, ChiakiAudioFrameCallback frame_cb, void *user)
{
	session->audio_settings_cb = settings_cb;
	session->audio_frame_cb = frame_cb;
	session->audio_cb_user = user;
}

static inline void chiaki_session_set_video_sample_cb(ChiakiSession *session, ChiakiVideoSampleCallback cb, void *user)
{
	session->video_sample_cb = cb;
	session->video_sample_cb_user = user;
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_SESSION_H
