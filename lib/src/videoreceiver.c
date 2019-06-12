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

#include <chiaki/videoreceiver.h>
#include <chiaki/session.h>

#include <string.h>

CHIAKI_EXPORT void chiaki_video_receiver_init(ChiakiVideoReceiver *video_receiver, struct chiaki_session_t *session)
{
	video_receiver->session = session;
	video_receiver->log = &session->log;
	memset(video_receiver->profiles, 0, sizeof(video_receiver->profiles));
	video_receiver->profiles_count = 0;
	video_receiver->profile_cur = -1;

	video_receiver->frame_index_cur = -1;
	video_receiver->frame_index_prev = -1;

	chiaki_frame_processor_init(&video_receiver->frame_processor, video_receiver->log);
}

CHIAKI_EXPORT void chiaki_video_receiver_fini(ChiakiVideoReceiver *video_receiver)
{
	for(size_t i=0; i<video_receiver->profiles_count; i++)
		free(video_receiver->profiles[i].header);
	chiaki_frame_processor_fini(&video_receiver->frame_processor);
}

CHIAKI_EXPORT void chiaki_video_receiver_stream_info(ChiakiVideoReceiver *video_receiver, ChiakiVideoProfile *profiles, size_t profiles_count)
{
	if(video_receiver->profiles_count > 0)
	{
		CHIAKI_LOGE(video_receiver->log, "Video Receiver profiles already set\n");
		return;
	}

	memcpy(video_receiver->profiles, profiles, profiles_count * sizeof(ChiakiVideoProfile));
	video_receiver->profiles_count = profiles_count;

	CHIAKI_LOGI(video_receiver->log, "Video Profiles:\n");
	for(size_t i=0; i<video_receiver->profiles_count; i++)
	{
		ChiakiVideoProfile *profile = &video_receiver->profiles[i];
		CHIAKI_LOGI(video_receiver->log, "  %zu: %ux%u\n", i, profile->width, profile->height);
		chiaki_log_hexdump(video_receiver->log, CHIAKI_LOG_DEBUG, profile->header, profile->header_sz);
	}
}

CHIAKI_EXPORT void chiaki_video_receiver_av_packet(ChiakiVideoReceiver *video_receiver, ChiakiTakionAVPacket *packet)
{
	ChiakiSeqNum16 frame_index = packet->frame_index;
	if(video_receiver->frame_index_cur >= 0
		&& chiaki_seq_num_16_lt(frame_index, (ChiakiSeqNum16)video_receiver->frame_index_cur))
	{
		CHIAKI_LOGW(video_receiver->log, "Video Receiver received old frame packet\n");
		return;
	}

	if(video_receiver->profile_cur < 0 || video_receiver->profile_cur != packet->adaptive_stream_index)
	{
		if(packet->adaptive_stream_index >= video_receiver->profiles_count)
		{
			CHIAKI_LOGE(video_receiver->log, "Packet has invalid adaptive stream index %lu >= %lu\n",
					(unsigned int)packet->adaptive_stream_index,
					(unsigned int)video_receiver->profiles_count);
			return;
		}
		video_receiver->profile_cur = packet->adaptive_stream_index;

		ChiakiVideoProfile *profile = video_receiver->profiles + video_receiver->profile_cur;
		CHIAKI_LOGI(video_receiver->log, "Switched to profile %d, resolution: %ux%u\n", video_receiver->profile_cur, profile->width, profile->height);
		if(video_receiver->session->video_sample_cb)
			video_receiver->session->video_sample_cb(profile->header, profile->header_sz, video_receiver->session->video_sample_cb_user);
	}

	if(video_receiver->frame_index_cur < 0 ||
		chiaki_seq_num_16_gt(frame_index, (ChiakiSeqNum16)video_receiver->frame_index_cur))
	{
		if(video_receiver->frame_index_cur >= 0 && video_receiver->frame_index_prev != video_receiver->frame_index_cur)
		{
			uint8_t *frame;
			size_t frame_size;
			if(chiaki_frame_processor_flush(&video_receiver->frame_processor, &frame, &frame_size) == CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_SUCCESS)
			{
				CHIAKI_LOGD(video_receiver->log, "Decoded frame %d\n", (int)video_receiver->frame_index_cur);
				if(video_receiver->session->video_sample_cb)
					video_receiver->session->video_sample_cb(frame, frame_size, video_receiver->session->video_sample_cb_user);
				free(frame);
			}
			else
			{
				// TODO: fake frame?
				CHIAKI_LOGW(video_receiver->log, "Failed to complete frame %d\n", (int)video_receiver->frame_index_cur);
			}

			video_receiver->frame_index_prev = video_receiver->frame_index_cur;
		}

		if(chiaki_seq_num_16_gt(frame_index, (ChiakiSeqNum16)video_receiver->frame_index_cur + 1)
			&& !(frame_index == 1 && video_receiver->frame_index_cur < 0)) // ok for frame 1
		{
			CHIAKI_LOGW(video_receiver->log, "Skipped from frame %d to %d\n", (int)video_receiver->frame_index_cur, (int)frame_index);
			// TODO: fake frame?
		}

		video_receiver->frame_index_cur = frame_index;
		CHIAKI_LOGD(video_receiver->log, "Preparing slots for frame %d\n", (int)video_receiver->frame_index_cur);
		chiaki_frame_processor_alloc_frame(&video_receiver->frame_processor, packet);
	}

	//CHIAKI_LOGD(video_receiver->log, "Putting unit %lu of frame %d in processor\n",
	//		(unsigned int)packet->unit_index, (int)video_receiver->frame_index_cur);
	chiaki_frame_processor_put_unit(&video_receiver->frame_processor, packet);
}
