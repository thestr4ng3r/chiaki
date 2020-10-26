// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_REORDERQUEUE_H
#define CHIAKI_REORDERQUEUE_H

#include <stdlib.h>

#include "seqnum.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum chiaki_reorder_queue_drop_strategy_t {
	CHIAKI_REORDER_QUEUE_DROP_STRATEGY_BEGIN, // drop packet with lowest number
	CHIAKI_REORDER_QUEUE_DROP_STRATEGY_END // drop packet with highest number
} ChiakiReorderQueueDropStrategy;

typedef struct chiaki_reorder_queue_entry_t
{
	void *user;
	bool set;
} ChiakiReorderQueueEntry;

typedef void (*ChiakiReorderQueueDropCb)(uint64_t seq_num, void *elem_user, void *cb_user);
typedef bool (*ChiakiReorderQueueSeqNumGt)(uint64_t a, uint64_t b);
typedef bool (*ChiakiReorderQueueSeqNumLt)(uint64_t a, uint64_t b);
typedef uint64_t (*ChiakiReorderQueueSeqNumAdd)(uint64_t a, uint64_t b);
typedef uint64_t (*ChiakiReorderQueueSeqNumSub)(uint64_t a, uint64_t b);

typedef struct chiaki_reorder_queue_t
{
	size_t size_exp; // real size = 2^size * sizeof(ChiakiReorderQueueEntry)
	ChiakiReorderQueueEntry *queue;
	uint64_t begin;
	uint64_t count;
	ChiakiReorderQueueSeqNumGt seq_num_gt;
	ChiakiReorderQueueSeqNumLt seq_num_lt;
	ChiakiReorderQueueSeqNumAdd seq_num_add;
	ChiakiReorderQueueSeqNumSub seq_num_sub;
	ChiakiReorderQueueDropStrategy drop_strategy;
	ChiakiReorderQueueDropCb drop_cb;
	void *drop_cb_user;
} ChiakiReorderQueue;

/**
 * @param size exponent for 2
 * @param seq_num_start sequence number of the first expected element
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_reorder_queue_init(ChiakiReorderQueue *queue, size_t size_exp,
		uint64_t seq_num_start, ChiakiReorderQueueSeqNumGt seq_num_gt, ChiakiReorderQueueSeqNumLt seq_num_lt, ChiakiReorderQueueSeqNumAdd seq_num_add);

/**
 * Helper to initialize a queue using ChiakiSeqNum16 sequence numbers
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_reorder_queue_init_16(ChiakiReorderQueue *queue, size_t size_exp, ChiakiSeqNum16 seq_num_start);

/**
 * Helper to initialize a queue using ChiakiSeqNum32 sequence numbers
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_reorder_queue_init_32(ChiakiReorderQueue *queue, size_t size_exp, ChiakiSeqNum32 seq_num_start);

CHIAKI_EXPORT void chiaki_reorder_queue_fini(ChiakiReorderQueue *queue);

static inline void chiaki_reorder_queue_set_drop_strategy(ChiakiReorderQueue *queue, ChiakiReorderQueueDropStrategy drop_strategy)
{
	queue->drop_strategy = drop_strategy;
}

static inline void chiaki_reorder_queue_set_drop_cb(ChiakiReorderQueue *queue, ChiakiReorderQueueDropCb cb, void *user)
{
	queue->drop_cb = cb;
	queue->drop_cb_user = user;
}

static inline size_t chiaki_reorder_queue_size(ChiakiReorderQueue *queue)
{
	return ((size_t)1) << queue->size_exp;
}

static inline uint64_t chiaki_reorder_queue_count(ChiakiReorderQueue *queue)
{
	return queue->count;
}

/**
 * Push a packet into the queue.
 *
 * Depending on the set drop strategy, this might drop elements and call the drop callback with the dropped elements.
 * The callback will also be called with the new element íf there is already an element with the same sequence number
 * or if the sequence number is less than queue->begin, i.e. the next element to be pulled.
 *
 * @param seq_num
 * @param user pointer to be associated with the element
 */
CHIAKI_EXPORT void chiaki_reorder_queue_push(ChiakiReorderQueue *queue, uint64_t seq_num, void *user);

/**
 * Pull the next element in order from the queue.
 *
 * Call this repeatedly until it returns false to get all subsequently available elements.
 *
 * @param seq_num pointer where the sequence number of the pulled packet is written, undefined contents if false is returned
 * @param user pointer where the user pointer of the pulled packet is written, undefined contents if false is returned
 * @return true if an element was pulled in order
 */
CHIAKI_EXPORT bool chiaki_reorder_queue_pull(ChiakiReorderQueue *queue, uint64_t *seq_num, void **user);

/**
 * Peek the element at a specific index inside the queue.
 *
 * @param index Offset to be added to the begin sequence number, this is NOT a sequence number itself! (0 <= index < count)
 * @param seq_num pointer where the sequence number of the peeked packet is written, undefined contents if false is returned
 * @param user pointer where the user pointer of the pulled packet is written, undefined contents if false is returned
 * @return true if an element was peeked, false if there is no element at index.
 */
CHIAKI_EXPORT bool chiaki_reorder_queue_peek(ChiakiReorderQueue *queue, uint64_t index, uint64_t *seq_num, void **user);

/**
 * Drop a specific element from the queue.
 * begin will not be changed.
 * @param index Offset to be added to the begin sequence number, this is NOT a sequence number itself! (0 <= index < count)
 */
CHIAKI_EXPORT void chiaki_reorder_queue_drop(ChiakiReorderQueue *queue, uint64_t index);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_REORDERQUEUE_H
