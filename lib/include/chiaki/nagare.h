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

#ifndef CHIAKI_NAGARE_H
#define CHIAKI_NAGARE_H

#include "mirai.h"
#include "takion.h"
#include "log.h"
#include "ecdh.h"
#include "gkcrypt.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_nagare_t
{
	struct chiaki_session_t *session;
	ChiakiLog *log;
	ChiakiTakion takion;
	ChiakiMirai mirai;
	uint8_t *ecdh_secret;
	ChiakiGKCrypt *gkcrypt_local;
	ChiakiGKCrypt *gkcrypt_remote;
} ChiakiNagare;

CHIAKI_EXPORT ChiakiErrorCode chiaki_nagare_run(struct chiaki_session_t *session);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_NAGARE_H
