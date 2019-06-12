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

#ifndef CHIAKI_SEQNUM_H
#define CHIAKI_SEQNUM_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ChiakiSeqNum16;

#define CHIAKI_SEQ_NUM_16_MAX 0xffff

static inline bool chiaki_seq_num_16_lt(ChiakiSeqNum16 a, ChiakiSeqNum16 b)
{
	if(a == b)
		return false;
	int32_t d = (int32_t)b - (int32_t)a;
	return (a < b && d < (1 << 15))
		|| ((a > b) && -d > (1 << 15));
}

static inline bool chiaki_seq_num_16_gt(ChiakiSeqNum16 a, ChiakiSeqNum16 b)
{
	if(a == b)
		return false;
	int32_t d = (int32_t)b - (int32_t)a;
	return (a < b && d > (1 << 15))
		   || ((a > b) && -d < (1 << 15));
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_SEQNUM_H
