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

#ifndef CHIAKI_SENKUSHA_H
#define CHIAKI_SENKUSHA_H

#include "takion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_session_t ChiakiSession;

typedef struct senkusha_t
{
	ChiakiSession *session;
	ChiakiLog *log;
	ChiakiTakion takion;

	int state;
	bool state_finished;
	bool state_failed;
	bool should_stop;
	ChiakiSeqNum32 data_ack_seq_num_expected;
	uint64_t pong_time_us;
	uint16_t ping_test_index;
	uint16_t ping_index;
	uint32_t ping_tag;
	uint32_t mtu_id;

	/**
	 * signaled on change of state_finished or should_stop
	 */
	ChiakiCond state_cond;

	/**
	 * protects state, state_finished, state_failed and should_stop
	 */
	ChiakiMutex state_mutex;
} ChiakiSenkusha;

CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_init(ChiakiSenkusha *senkusha, ChiakiSession *session);
CHIAKI_EXPORT void chiaki_senkusha_fini(ChiakiSenkusha *senkusha);
CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_run(ChiakiSenkusha *senkusha, uint32_t *mtu_in, uint32_t *mtu_out, uint64_t *rtt_us);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_SENKUSHA_H
