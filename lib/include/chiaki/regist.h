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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_regist_info_t
{
	char *host;
	char *psn_id;
	uint32_t pin;
} ChiakiRegistInfo;

typedef enum chiaki_regist_event_type_t {
	CHIAKI_REGIST_EVENT_TYPE_CONNECTED,
	CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED,
	CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS
} ChiakiRegistEventType;

typedef struct chiaki_regist_event_t
{
	ChiakiRegistEventType type;
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

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_REGIST_H
