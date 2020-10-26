// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

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
