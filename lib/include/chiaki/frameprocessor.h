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

#ifndef CHIAKI_FRAMEPROCESSOR_H
#define CHIAKI_FRAMEPROCESSOR_H

#include "common.h"
#include "takion.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct chiaki_frame_unit_t;
typedef struct chiaki_frame_unit_t ChiakiFrameUnit;

typedef struct chiaki_frame_processor_t
{
	ChiakiLog *log;
	uint8_t *frame_buf;
	size_t frame_buf_size;
	size_t buf_size_per_unit;
	size_t buf_stride_per_unit;
	unsigned int units_source_expected;
	unsigned int units_fec_expected;
	unsigned int units_source_received;
	unsigned int units_fec_received;
	ChiakiFrameUnit *unit_slots;
	size_t unit_slots_size;
} ChiakiFrameProcessor;

typedef enum chiaki_frame_flush_result_t {
	CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_SUCCESS = 0,
	CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FEC_SUCCESS = 1,
	CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FEC_FAILED = 2,
	CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FAILED = 3
} ChiakiFrameProcessorFlushResult;

CHIAKI_EXPORT void chiaki_frame_processor_init(ChiakiFrameProcessor *frame_processor, ChiakiLog *log);
CHIAKI_EXPORT void chiaki_frame_processor_fini(ChiakiFrameProcessor *frame_processor);

CHIAKI_EXPORT ChiakiErrorCode chiaki_frame_processor_alloc_frame(ChiakiFrameProcessor *frame_processor, ChiakiTakionAVPacket *packet);
CHIAKI_EXPORT ChiakiErrorCode chiaki_frame_processor_put_unit(ChiakiFrameProcessor *frame_processor, ChiakiTakionAVPacket *packet);

/**
 * @param frame unless CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FAILED returned, will receive a pointer into the internal buffer of frame_processor.
 * MUST NOT be used after the next call to this frame processor!
 */
CHIAKI_EXPORT ChiakiFrameProcessorFlushResult chiaki_frame_processor_flush(ChiakiFrameProcessor *frame_processor, uint8_t **frame, size_t *frame_size);

static inline bool chiaki_frame_processor_flush_possible(ChiakiFrameProcessor *frame_processor)
{
	return frame_processor->units_source_received + frame_processor->units_fec_received
		>= frame_processor->units_source_expected;
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_FRAMEPROCESSOR_H
