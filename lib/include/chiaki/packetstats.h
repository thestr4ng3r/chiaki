// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_PACKETSTATS_H
#define CHIAKI_PACKETSTATS_H

#include "thread.h"
#include "seqnum.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_packet_stats_t
{
	ChiakiMutex mutex;

	// For generations of packets, i.e. where we know the number of expected packets per generation
	uint64_t gen_received;
	uint64_t gen_lost;

	// For sequential packets, i.e. where packets are identified by a sequence number
	ChiakiSeqNum16 seq_min; // sequence number that was max at the last reset
	ChiakiSeqNum16 seq_max; // currently maximal sequence number
	uint64_t seq_received; // total received packets since the last reset
} ChiakiPacketStats;

CHIAKI_EXPORT ChiakiErrorCode chiaki_packet_stats_init(ChiakiPacketStats *stats);
CHIAKI_EXPORT void chiaki_packet_stats_fini(ChiakiPacketStats *stats);
CHIAKI_EXPORT void chiaki_packet_stats_reset(ChiakiPacketStats *stats);
CHIAKI_EXPORT void chiaki_packet_stats_push_generation(ChiakiPacketStats *stats, uint64_t received, uint64_t lost);
CHIAKI_EXPORT void chiaki_packet_stats_push_seq(ChiakiPacketStats *stats, ChiakiSeqNum16 seq_num);
CHIAKI_EXPORT void chiaki_packet_stats_get(ChiakiPacketStats *stats, bool reset, uint64_t *received, uint64_t *lost);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_PACKETSTATS_H
