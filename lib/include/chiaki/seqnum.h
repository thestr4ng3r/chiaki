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

// RFC 1982

#define CHIAKI_DEFINE_SEQNUM(bits, greater_sint) \
\
typedef uint##bits##_t ChiakiSeqNum##bits; \
\
static inline bool chiaki_seq_num_##bits##_lt(ChiakiSeqNum##bits a, ChiakiSeqNum##bits b) \
{ \
	if(a == b) \
		return false; \
	greater_sint d = (greater_sint)b - (greater_sint)a; \
	return (a < b && d < ((ChiakiSeqNum##bits)1 << (bits - 1))) \
		|| ((a > b) && -d > ((ChiakiSeqNum##bits)1 << (bits - 1))); \
} \
\
static inline bool chiaki_seq_num_##bits##_gt(ChiakiSeqNum##bits a, ChiakiSeqNum##bits b) \
{ \
	if(a == b) \
		return false; \
	greater_sint d = (greater_sint)b - (greater_sint)a; \
	return (a < b && d > ((ChiakiSeqNum##bits)1 << (bits - 1))) \
		   || ((a > b) && -d < ((ChiakiSeqNum##bits)1 << (bits - 1))); \
}

CHIAKI_DEFINE_SEQNUM(16, int32_t)
CHIAKI_DEFINE_SEQNUM(32, int64_t)
#undef CHIAKI_DEFINE_SEQNUM

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_SEQNUM_H
