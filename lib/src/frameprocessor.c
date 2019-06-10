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

#include <chiaki/frameprocessor.h>

#include <string.h>


#define UNIT_SLOTS_MAX 256


struct chiaki_frame_unit_t
{
	size_t data_size;
};


CHIAKI_EXPORT void chiaki_frame_processor_init(ChiakiFrameProcessor *frame_processor, ChiakiLog *log)
{
	frame_processor->log = log;
	frame_processor->frame_buf = NULL;
	frame_processor->frame_buf_size = 0;
	frame_processor->units_regular_expected = 0;
	frame_processor->units_additional_expected = 0;
	frame_processor->unit_slots = NULL;
	frame_processor->unit_slots_size = 0;
}

CHIAKI_EXPORT void chiaki_frame_processor_fini(ChiakiFrameProcessor *frame_processor)
{
	free(frame_processor->frame_buf);
	free(frame_processor->unit_slots);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_frame_processor_alloc_frame(ChiakiFrameProcessor *frame_processor, ChiakiTakionAVPacket *packet)
{
	if(packet->units_in_frame_total < packet->units_in_frame_additional)
	{
		CHIAKI_LOGE(frame_processor->log, "Packet has units_in_frame_total < units_in_frame_additional\n");
		return CHIAKI_ERR_INVALID_DATA;
	}

	frame_processor->units_regular_expected = packet->units_in_frame_total - packet->units_in_frame_additional;
	frame_processor->units_additional_expected = packet->units_in_frame_additional;
	if(frame_processor->units_additional_expected < 1)
		frame_processor->units_additional_expected = 1;

	frame_processor->buf_size_per_unit = packet->data_size;
	if(packet->is_video && packet->unit_index < frame_processor->units_regular_expected)
	{
		if(packet->data_size < 2)
		{
			CHIAKI_LOGE(frame_processor->log, "Packet too small to read buf size extension\n");
			return CHIAKI_ERR_BUF_TOO_SMALL;
		}
		frame_processor->buf_size_per_unit += ntohs(((uint16_t *)packet->data)[0]);
	}

	if(frame_processor->buf_size_per_unit == 0)
	{
		CHIAKI_LOGE(frame_processor->log, "Frame Processor doesn't handle empty units\n");
		return CHIAKI_ERR_BUF_TOO_SMALL;
	}

	frame_processor->units_regular_received = 0;
	frame_processor->units_additional_received = 0;

	size_t unit_slots_size_required = frame_processor->units_regular_expected + frame_processor->units_additional_expected;
	if(unit_slots_size_required > UNIT_SLOTS_MAX)
	{
		CHIAKI_LOGE(frame_processor->log, "Packet suggests more than %u unit slots\n", UNIT_SLOTS_MAX);
		return CHIAKI_ERR_INVALID_DATA;
	}
	if(unit_slots_size_required != frame_processor->unit_slots_size)
	{
		void *new_ptr = NULL;
		if(frame_processor->unit_slots)
		{
			new_ptr = realloc(frame_processor->unit_slots, unit_slots_size_required * sizeof(ChiakiFrameUnit));
			if(!new_ptr)
				free(frame_processor->unit_slots);
		}
		else
			new_ptr = malloc(unit_slots_size_required * sizeof(ChiakiFrameUnit));

		frame_processor->unit_slots = new_ptr;
		if(!new_ptr)
		{
			frame_processor->unit_slots_size = 0;
			return CHIAKI_ERR_MEMORY;
		}
		else
			frame_processor->unit_slots_size = unit_slots_size_required;
	}
	memset(frame_processor->unit_slots, 0, frame_processor->unit_slots_size * sizeof(ChiakiFrameUnit));

	if(frame_processor->unit_slots_size > SIZE_MAX / frame_processor->buf_size_per_unit)
		return CHIAKI_ERR_OVERFLOW;
	size_t frame_buf_size_required = frame_processor->unit_slots_size * frame_processor->buf_size_per_unit;
	if(frame_processor->frame_buf_size < frame_buf_size_required)
	{
		free(frame_processor->frame_buf);
		frame_processor->frame_buf = malloc(frame_buf_size_required);
		if(!frame_processor->frame_buf)
		{
			frame_processor->frame_buf_size = 0;
			return CHIAKI_ERR_MEMORY;
		}
		frame_processor->frame_buf_size = frame_buf_size_required;
	}
	memset(frame_processor->frame_buf, 0, frame_buf_size_required);

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_frame_processor_put_unit(ChiakiFrameProcessor *frame_processor, ChiakiTakionAVPacket *packet)
{
	if(packet->unit_index > frame_processor->unit_slots_size)
	{
		CHIAKI_LOGE(frame_processor->log, "Packet's unit index is too high\n");
		return CHIAKI_ERR_INVALID_DATA;
	}
	
	if(!packet->data_size)
	{
		CHIAKI_LOGW(frame_processor->log, "Unit is empty\n");
		return CHIAKI_ERR_INVALID_DATA;
	}

	if(packet->data_size > frame_processor->buf_size_per_unit)
	{
		CHIAKI_LOGW(frame_processor->log, "Unit is bigger than pre-calculated size!\n");
		return CHIAKI_ERR_INVALID_DATA;
	}
	
	ChiakiFrameUnit *unit = frame_processor->unit_slots + packet->unit_index;
	if(unit->data_size)
	{
		CHIAKI_LOGW(frame_processor->log, "Received duplicate unit\n");
		return CHIAKI_ERR_INVALID_DATA;
	}

	unit->data_size = packet->data_size;
	memcpy(frame_processor->frame_buf + packet->unit_index, packet->data, packet->data_size);

	if(packet->unit_index < frame_processor->units_regular_expected)
		frame_processor->units_regular_received++;
	else
		frame_processor->units_additional_received++;

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiFrameProcessorFlushResult chiaki_frame_processor_flush(ChiakiFrameProcessor *frame_processor, uint8_t **frame, size_t *frame_size)
{
	if(frame_processor->units_regular_expected == 0)
		return CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FAILED;

	// TODO: FEC
	if(frame_processor->units_regular_received < frame_processor->units_regular_expected)
		return CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FAILED;

	uint8_t *buf = malloc(frame_processor->frame_buf_size); // TODO: this should come from outside instead of mallocing all the time
	if(!buf)
		return CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FAILED;

	size_t buf_size = 0;
	for(size_t i=0; i<frame_processor->units_regular_expected; i++)
	{
		ChiakiFrameUnit *unit = frame_processor->unit_slots + i;
		if(unit->data_size < 2)
		{
			CHIAKI_LOGE(frame_processor->log, "Saved unit has size < 2\n");
			continue;
		}
		size_t part_size = unit->data_size - 2;
		memcpy(buf + buf_size, frame_processor->frame_buf + i*frame_processor->buf_size_per_unit + 2, part_size);
		buf_size += part_size;
	}

	*frame = buf;
	*frame_size = buf_size;
	return CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_SUCCESS;
}
