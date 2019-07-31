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

#ifndef CHIAKI_UNIT_TEST

#include <chiaki/takionsendbuffer.h>
#include <chiaki/takion.h>
#include <chiaki/time.h>

#include <string.h>
#include <assert.h>

#define TAKION_DATA_RESEND_TIMEOUT_MS 200
#define TAKION_DATA_RESEND_WAKEUP_TIMEOUT_MS (TAKION_DATA_RESEND_TIMEOUT_MS/2)
#define TAKION_DATA_RESEND_TRIES_MAX 10

#endif

struct chiaki_takion_send_buffer_packet_t
{
	ChiakiSeqNum32 seq_num;
	uint64_t tries;
	uint64_t last_send_ms; // chiaki_time_now_monotonic_ms()
	uint8_t *buf;
	size_t buf_size;
}; // ChiakiTakionSendBufferPacket

#ifndef CHIAKI_UNIT_TEST

static void *takion_send_buffer_thread_func(void *user);

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_buffer_init(ChiakiTakionSendBuffer *send_buffer, ChiakiTakion *takion, size_t size)
{
	send_buffer->takion = takion;
	send_buffer->log = takion ? takion->log : NULL;

	send_buffer->packets = calloc(size, sizeof(ChiakiTakionSendBufferPacket));
	if(!send_buffer->packets)
		return CHIAKI_ERR_MEMORY;
	send_buffer->packets_size = size;
	send_buffer->packets_count = 0;

	send_buffer->should_stop = false;

	ChiakiErrorCode err = chiaki_mutex_init(&send_buffer->mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_packets;

	err = chiaki_cond_init(&send_buffer->cond);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_mutex;

	err = chiaki_thread_create(&send_buffer->thread, takion_send_buffer_thread_func, send_buffer);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_cond;

	return CHIAKI_ERR_SUCCESS;
error_cond:
	chiaki_cond_fini(&send_buffer->cond);
error_mutex:
	chiaki_mutex_fini(&send_buffer->mutex);
error_packets:
	free(send_buffer->packets);
	return err;
}

CHIAKI_EXPORT void chiaki_takion_send_buffer_fini(ChiakiTakionSendBuffer *send_buffer)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&send_buffer->mutex);
	assert(err == CHIAKI_ERR_SUCCESS);
	send_buffer->should_stop = true;
	chiaki_mutex_unlock(&send_buffer->mutex);
	err = chiaki_cond_signal(&send_buffer->cond);
	assert(err == CHIAKI_ERR_SUCCESS);
	err = chiaki_thread_join(&send_buffer->thread, NULL);
	assert(err == CHIAKI_ERR_SUCCESS);

	for(size_t i=0; i<send_buffer->packets_count; i++)
		free(send_buffer->packets[i].buf);

	chiaki_cond_fini(&send_buffer->cond);
	chiaki_mutex_fini(&send_buffer->mutex);
	free(send_buffer->packets);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_buffer_push(ChiakiTakionSendBuffer *send_buffer, ChiakiSeqNum32 seq_num, uint8_t *buf, size_t buf_size)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&send_buffer->mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	if(send_buffer->packets_count >= send_buffer->packets_size)
	{
		CHIAKI_LOGE(send_buffer->log, "Takion Send Buffer overflow");
		err = CHIAKI_ERR_OVERFLOW;
		goto beach;
	}

	for(size_t i=0; i<send_buffer->packets_count; i++)
	{
		if(send_buffer->packets[i].seq_num == seq_num)
		{
			CHIAKI_LOGE(send_buffer->log, "Tried to push duplicate seqnum into Takion Send Buffer");
			err = CHIAKI_ERR_INVALID_DATA;
			goto beach;
		}
	}

	ChiakiTakionSendBufferPacket *packet = &send_buffer->packets[send_buffer->packets_count++];
	packet->seq_num = seq_num;
	packet->tries = 0;
	packet->last_send_ms = chiaki_time_now_monotonic_ms();
	packet->buf = buf;
	packet->buf_size = buf_size;

	CHIAKI_LOGD(send_buffer->log, "Pushed seq num %#llx into Takion Send Buffer", (unsigned long long)seq_num);

	if(send_buffer->packets_count == 1)
	{
		// buffer was empty before, so it will sleep without timeout => WAKE UP!!
		chiaki_cond_signal(&send_buffer->cond);
	}

beach:
	if(err != CHIAKI_ERR_SUCCESS)
		free(buf);
	chiaki_mutex_unlock(&send_buffer->mutex);
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_buffer_ack(ChiakiTakionSendBuffer *send_buffer, ChiakiSeqNum32 seq_num)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&send_buffer->mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	size_t i;
	size_t shift = 0; // amount to shift back
	size_t shift_start = SIZE_MAX;
	for(i=0; i<send_buffer->packets_count; i++)
	{
		if(send_buffer->packets[i].seq_num == seq_num || chiaki_seq_num_32_lt(send_buffer->packets[i].seq_num, seq_num))
		{
			free(send_buffer->packets[i].buf);
			if(shift_start == SIZE_MAX)
			{
				// first shift
				shift_start = i;
				shift = 1;
			}
			else if(shift_start + shift == i)
			{
				// still in the same gap
				shift++;
			}
			else
			{
				// new gap, do shift
				memmove(send_buffer->packets + shift_start,
						send_buffer->packets + shift_start + shift,
						(i - (shift_start + shift)) * sizeof(ChiakiTakionSendBufferPacket));
				// start new shift
				shift_start = i - shift;
				shift++;
			}
		}
	}

	if(shift_start != SIZE_MAX)
	{
		// do final shift
		if(shift_start + shift < send_buffer->packets_count)
		{
			memmove(send_buffer->packets + shift_start,
					send_buffer->packets + shift_start + shift,
					(send_buffer->packets_count - (shift_start + shift)) * sizeof(ChiakiTakionSendBufferPacket));
		}
		send_buffer->packets_count -= shift;
	}

	CHIAKI_LOGD(send_buffer->log, "Acked seq num %#llx from Takion Send Buffer", (unsigned long long)seq_num);

	chiaki_mutex_unlock(&send_buffer->mutex);
	return err;
}

static void takion_send_buffer_resend(ChiakiTakionSendBuffer *send_buffer);

static bool takion_send_buffer_check_pred_packets(void *user)
{
	ChiakiTakionSendBuffer *send_buffer = user;
	return send_buffer->should_stop;
}

static bool takion_send_buffer_check_pred_no_packets(void *user)
{
	ChiakiTakionSendBuffer *send_buffer = user;
	return send_buffer->should_stop || send_buffer->packets_count;
}

static void *takion_send_buffer_thread_func(void *user)
{
	ChiakiTakionSendBuffer *send_buffer = user;

	ChiakiErrorCode err = chiaki_mutex_lock(&send_buffer->mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return NULL;

	while(true)
	{
		if(send_buffer->packets_count) // if there are packets, wait with timeout
			err = chiaki_cond_timedwait_pred(&send_buffer->cond, &send_buffer->mutex, TAKION_DATA_RESEND_WAKEUP_TIMEOUT_MS, takion_send_buffer_check_pred_packets, send_buffer);
		else // if not, wait without timeout, but also wakeup if packets become available
			err = chiaki_cond_wait_pred(&send_buffer->cond, &send_buffer->mutex, takion_send_buffer_check_pred_no_packets, send_buffer);

		if(err != CHIAKI_ERR_SUCCESS && err != CHIAKI_ERR_TIMEOUT)
			break;

		if(send_buffer->should_stop)
			break;

		takion_send_buffer_resend(send_buffer);
	}

	chiaki_mutex_unlock(&send_buffer->mutex);

	return NULL;
}

static void takion_send_buffer_resend(ChiakiTakionSendBuffer *send_buffer)
{
	if(!send_buffer->takion)
		return;

	uint64_t now = chiaki_time_now_monotonic_ms();

	for(size_t i=0; i<send_buffer->packets_count; i++)
	{
		ChiakiTakionSendBufferPacket *packet = &send_buffer->packets[i];
		if(now - packet->last_send_ms > TAKION_DATA_RESEND_TIMEOUT_MS)
		{
			CHIAKI_LOGI(send_buffer->log, "Takion Send Buffer re-sending packet with seqnum %#llx, tries: %llu", (unsigned long long)packet->seq_num, (unsigned long long)packet->tries);
			packet->last_send_ms = now;
			chiaki_takion_send_raw(send_buffer->takion, packet->buf, packet->buf_size);
			packet->tries++;
			// TODO: check tries and disconnect if necessary
		}
	}
}

#endif
