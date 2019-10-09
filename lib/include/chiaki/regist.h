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

#ifndef CHIAKI_REGIST_H
#define CHIAKI_REGIST_H

#include "common.h"
#include "log.h"
#include "thread.h"
#include "stoppipe.h"
#include "rpcrypt.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_PSN_ACCOUNT_ID_SIZE 8

typedef struct chiaki_regist_info_t
{
	const char *host;
	bool broadcast;

	/**
	 * may be null, in which case psn_account_id will be used
	 */
	const char *psn_online_id;

	/**
	 * will be used if psn_online_id is null, for PS4 >= 7.0
	 */
	uint8_t psn_account_id[CHIAKI_PSN_ACCOUNT_ID_SIZE];

	uint32_t pin;
} ChiakiRegistInfo;

typedef struct chiaki_registered_host_t
{
	char ap_ssid[0x30];
	char ap_bssid[0x20];
	char ap_key[0x50];
	char ap_name[0x20];
	uint8_t ps4_mac[6];
	char ps4_nickname[0x20];
	char rp_regist_key[CHIAKI_SESSION_AUTH_SIZE]; // must be completely filled (pad with \0)
	uint32_t rp_key_type;
	uint8_t rp_key[0x10];
} ChiakiRegisteredHost;

typedef enum chiaki_regist_event_type_t {
	CHIAKI_REGIST_EVENT_TYPE_FINISHED_CANCELED,
	CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED,
	CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS
} ChiakiRegistEventType;

typedef struct chiaki_regist_event_t
{
	ChiakiRegistEventType type;
	ChiakiRegisteredHost *registered_host;
} ChiakiRegistEvent;

typedef void (*ChiakiRegistCb)(ChiakiRegistEvent *event, void *user);

typedef struct chiaki_regist_t
{
	ChiakiLog *log;
	ChiakiRegistInfo info;
	ChiakiRegistCb cb;
	void *cb_user;

	ChiakiThread thread;
	ChiakiStopPipe stop_pipe;
} ChiakiRegist;

CHIAKI_EXPORT ChiakiErrorCode chiaki_regist_start(ChiakiRegist *regist, ChiakiLog *log, const ChiakiRegistInfo *info, ChiakiRegistCb cb, void *cb_user);
CHIAKI_EXPORT void chiaki_regist_fini(ChiakiRegist *regist);
CHIAKI_EXPORT void chiaki_regist_stop(ChiakiRegist *regist);

/**
 * @param psn_account_id must be exactly of size CHIAKI_PSN_ACCOUNT_ID_SIZE
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_regist_request_payload_format(uint8_t *buf, size_t *buf_size, ChiakiRPCrypt *crypt, const char *psn_online_id, const uint8_t *psn_account_id);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_REGIST_H
