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

#include <chiaki/reorderqueue.h>

#include <assert.h>

#define gt(a, b) (queue->seq_num_gt((a), (b)))
#define lt(a, b) (queue->seq_num_lt((a), (b)))
#define ge(a, b) ((a) == (b) || gt((a), (b)))
#define le(a, b) ((a) == (b) || lt((a), (b)))
#define add(a, b) (queue->seq_num_add((a), (b)))
#define QUEUE_SIZE (1 << queue->size_exp)
#define IDX_MASK ((1 << queue->size_exp) - 1)
#define idx(seq_num) ((seq_num) & IDX_MASK)

CHIAKI_EXPORT ChiakiErrorCode chiaki_reorder_queue_init(ChiakiReorderQueue *queue, size_t size_exp,
		uint64_t seq_num_start, ChiakiReorderQueueSeqNumGt seq_num_gt, ChiakiReorderQueueSeqNumLt seq_num_lt, ChiakiReorderQueueSeqNumAdd seq_num_add)
{
	queue->size_exp = size_exp;
	queue->begin = seq_num_start;
	queue->count = 0;
	queue->seq_num_gt = seq_num_gt;
	queue->seq_num_lt = seq_num_lt;
	queue->seq_num_add = seq_num_add;
	queue->drop_strategy = CHIAKI_REORDER_QUEUE_DROP_STRATEGY_END;
	queue->drop_cb = NULL;
	queue->drop_cb_user = NULL;
	queue->queue = calloc(1 << size_exp, sizeof(ChiakiReorderQueueEntry));
	if(!queue->queue)
		return CHIAKI_ERR_MEMORY;
	return CHIAKI_ERR_SUCCESS;
}

#define REORDER_QUEUE_INIT(bits) \
static bool seq_num_##bits##_gt(uint64_t a, uint64_t b) { return chiaki_seq_num_##bits##_gt((ChiakiSeqNum##bits)a, (ChiakiSeqNum##bits)b); } \
static bool seq_num_##bits##_lt(uint64_t a, uint64_t b) { return chiaki_seq_num_##bits##_lt((ChiakiSeqNum##bits)a, (ChiakiSeqNum##bits)b); } \
static uint64_t seq_num_##bits##_add(uint64_t a, uint64_t b) { return (uint64_t)((ChiakiSeqNum##bits)a + (ChiakiSeqNum##bits)b); } \
\
CHIAKI_EXPORT ChiakiErrorCode chiaki_reorder_queue_init_##bits(ChiakiReorderQueue *queue, size_t size_exp, ChiakiSeqNum##bits seq_num_start) \
{ \
	return chiaki_reorder_queue_init(queue, size_exp, (uint64_t)seq_num_start, \
			seq_num_##bits##_gt, seq_num_##bits##_lt, seq_num_##bits##_add); \
}

REORDER_QUEUE_INIT(16)
REORDER_QUEUE_INIT(32)

CHIAKI_EXPORT void chiaki_reorder_queue_fini(ChiakiReorderQueue *queue)
{
	free(queue->queue);
}

CHIAKI_EXPORT void chiaki_reorder_queue_push(ChiakiReorderQueue *queue, uint64_t seq_num, void *user)
{
	assert(queue->count <= QUEUE_SIZE);
	uint64_t end = add(queue->begin, queue->count);

	if(ge(seq_num, queue->begin) && lt(seq_num, end))
	{
		ChiakiReorderQueueEntry *entry = &queue->queue[idx(seq_num)];
		if(entry->set) // received twice
			goto drop_it;
		entry->user = user;
		entry->set = true;
		return;
	}

	if(lt(seq_num, queue->begin))
		goto drop_it;

	// => ge(seq_num, queue->end) == 1
	assert(ge(seq_num, end));

	uint64_t free_elems = QUEUE_SIZE - queue->count;
	uint64_t total_end = add(end, free_elems);
	uint64_t new_end = add(seq_num, 1);
	if(lt(total_end, new_end))
	{
		if(queue->drop_strategy == CHIAKI_REORDER_QUEUE_DROP_STRATEGY_END)
			goto drop_it;

		// drop first until empty or enough space
		while(queue->count > 0 && lt(total_end, new_end))
		{
			ChiakiReorderQueueEntry *entry = &queue->queue[idx(queue->begin)];
			if(entry->set)
				queue->drop_cb(queue->begin, entry->user, queue->drop_cb_user);
			queue->begin = add(queue->begin, 1);
			queue->count--;
			free_elems = QUEUE_SIZE - queue->count;
			total_end = add(end, free_elems);
		}

		// empty, just shift to the seq_num
		if(queue->count == 0)
			queue->begin = seq_num;
	}

	// move end until new_end
	end = add(queue->begin, queue->count);
	while(lt(end, new_end))
	{
		queue->count++;
		queue->queue[idx(end)].set = false;
		end = add(queue->begin, queue->count);
		assert(queue->count <= QUEUE_SIZE);
	}

	ChiakiReorderQueueEntry *entry = &queue->queue[idx(seq_num)];
	entry->set = true;
	entry->user = user;

	return;
drop_it:
	queue->drop_cb(seq_num, user, queue->drop_cb_user);
}

CHIAKI_EXPORT bool chiaki_reorder_queue_pull(ChiakiReorderQueue *queue, uint64_t *seq_num, void **user)
{
	assert(queue->count <= QUEUE_SIZE);
	if(queue->count == 0)
		return false;

	ChiakiReorderQueueEntry *entry = &queue->queue[idx(queue->begin)];
	if(!entry->set)
		return false;

	*seq_num = queue->begin;
	*user = entry->user;
	queue->begin = add(queue->begin, 1);
	queue->count--;
	return true;
}