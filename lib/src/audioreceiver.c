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

#include <chiaki/audioreceiver.h>
#include <chiaki/session.h>

#include <opus/opus.h>

#include <string.h>

static void chiaki_audio_receiver_frame(ChiakiAudioReceiver *audio_receiver, ChiakiSeqNum16 frame_index, uint8_t *buf, size_t buf_size);

CHIAKI_EXPORT ChiakiErrorCode chiaki_audio_receiver_init(ChiakiAudioReceiver *audio_receiver, ChiakiSession *session)
{
	audio_receiver->session = session;
	audio_receiver->log = session->log;
	audio_receiver->opus_decoder = NULL;
	memset(&audio_receiver->audio_header, 0, sizeof(audio_receiver->audio_header));

	audio_receiver->frame_index_prev = 0;
	audio_receiver->frame_index_startup = true;

	audio_receiver->pcm_buf = NULL;
	audio_receiver->pcm_buf_size = 0;

	ChiakiErrorCode err = chiaki_mutex_init(&audio_receiver->mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	return CHIAKI_ERR_SUCCESS;
}


CHIAKI_EXPORT void chiaki_audio_receiver_fini(ChiakiAudioReceiver *audio_receiver)
{
	opus_decoder_destroy(audio_receiver->opus_decoder);
	chiaki_mutex_fini(&audio_receiver->mutex);
	free(audio_receiver->pcm_buf);
}


CHIAKI_EXPORT void chiaki_audio_receiver_stream_info(ChiakiAudioReceiver *audio_receiver, ChiakiAudioHeader *audio_header)
{
	chiaki_mutex_lock(&audio_receiver->mutex);

	CHIAKI_LOGI(audio_receiver->log, "Audio Header:");
	CHIAKI_LOGI(audio_receiver->log, "  channels = %d", audio_header->channels);
	CHIAKI_LOGI(audio_receiver->log, "  bits = %d", audio_header->bits);
	CHIAKI_LOGI(audio_receiver->log, "  rate = %d", audio_header->rate);
	CHIAKI_LOGI(audio_receiver->log, "  frame size = %d", audio_header->frame_size);
	CHIAKI_LOGI(audio_receiver->log, "  unknown = %d", audio_header->unknown);
	memcpy(&audio_receiver->audio_header, audio_header, sizeof(audio_receiver->audio_header));

	opus_decoder_destroy(audio_receiver->opus_decoder);

	int error;
	audio_receiver->opus_decoder = opus_decoder_create(audio_header->rate, audio_header->channels, &error);

	if(error != OPUS_OK)
	{
		CHIAKI_LOGE(audio_receiver->log, "Audio Receiver failed to initialize opus decoder: %s", opus_strerror(error));
		goto beach;
	}

	CHIAKI_LOGI(audio_receiver->log, "Audio Receiver initialized opus decoder with the settings above");

	size_t pcm_buf_size_required = chiaki_audio_header_frame_buf_size(audio_header);
	int16_t *pcm_buf_old = audio_receiver->pcm_buf;
	if(!audio_receiver->pcm_buf || audio_receiver->pcm_buf_size != pcm_buf_size_required)
		audio_receiver->pcm_buf = realloc(audio_receiver->pcm_buf, pcm_buf_size_required);

	if(!audio_receiver->pcm_buf)
	{
		free(pcm_buf_old);
		CHIAKI_LOGE(audio_receiver->log, "Audio Receiver failed to alloc pcm buffer");
		opus_decoder_destroy(audio_receiver->opus_decoder);
		audio_receiver->opus_decoder = NULL;
		audio_receiver->pcm_buf_size = 0;
		goto beach;
	}

	audio_receiver->pcm_buf_size = pcm_buf_size_required;

beach:
	chiaki_mutex_unlock(&audio_receiver->mutex);
}

CHIAKI_EXPORT void chiaki_audio_receiver_av_packet(ChiakiAudioReceiver *audio_receiver, ChiakiTakionAVPacket *packet)
{
	if(packet->codec != 5)
	{
		CHIAKI_LOGE(audio_receiver->log, "Received Audio Packet with unknown Codec");
		return;
	}

	// this is mostly observation-based, so may not necessarily cover everything yet.

	uint8_t source_units_count = chiaki_takion_av_packet_audio_source_units_count(packet);
	uint8_t fec_units_count = chiaki_takion_av_packet_audio_fec_units_count(packet);
	uint8_t unit_size = chiaki_takion_av_packet_audio_unit_size(packet);

	if(!packet->data_size)
	{
		CHIAKI_LOGE(audio_receiver->log, "Audio AV Packet is empty");
		return;
	}

	if((uint16_t)fec_units_count + (uint16_t)source_units_count != packet->units_in_frame_total)
	{
		CHIAKI_LOGE(audio_receiver->log, "Source Units + FEC Units != Total Units in Audio AV Packet");
		return;
	}

	if(packet->data_size != (size_t)unit_size * (size_t)packet->units_in_frame_total)
	{
		CHIAKI_LOGE(audio_receiver->log, "Audio AV Packet size mismatch");
		return;
	}

	if(packet->frame_index > (1 << 15))
		audio_receiver->frame_index_startup = false;

	for(size_t i=0; i<source_units_count+fec_units_count; i++)
	{
		ChiakiSeqNum16 frame_index;
		if(i < source_units_count)
			frame_index = packet->frame_index + i;
		else
		{
			// fec
			size_t fec_index = i - source_units_count;

			// first packets will contain the same frame multiple times, ignore those
			if(audio_receiver->frame_index_startup && packet->frame_index + fec_index < fec_units_count + 1)
				continue;

			frame_index = packet->frame_index - fec_units_count + fec_index;
		}

		chiaki_audio_receiver_frame(audio_receiver->session->audio_receiver, frame_index, packet->data + unit_size * i, unit_size);
	}
}

static void chiaki_audio_receiver_frame(ChiakiAudioReceiver *audio_receiver, ChiakiSeqNum16 frame_index, uint8_t *buf, size_t buf_size)
{
	chiaki_mutex_lock(&audio_receiver->mutex);

	if(!audio_receiver->opus_decoder)
	{
		CHIAKI_LOGE(audio_receiver->log, "Received audio frame, but opus decoder is not initialized");
		goto beach;
	}

	if(!chiaki_seq_num_16_gt(frame_index, audio_receiver->frame_index_prev))
		goto beach;

	audio_receiver->frame_index_prev = frame_index;

	int r = opus_decode(audio_receiver->opus_decoder, buf, (opus_int32)buf_size, audio_receiver->pcm_buf, audio_receiver->audio_header.frame_size, 0);
	if(r < 1)
		CHIAKI_LOGE(audio_receiver->log, "Decoding audio frame with opus failed: %s", opus_strerror(r));
	else
		audio_receiver->session->audio_frame_cb(audio_receiver->pcm_buf, (size_t)r, audio_receiver->session->audio_frame_cb_user);

beach:
	chiaki_mutex_unlock(&audio_receiver->mutex);
}