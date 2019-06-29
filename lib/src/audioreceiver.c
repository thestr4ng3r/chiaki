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


CHIAKI_EXPORT ChiakiErrorCode chiaki_audio_receiver_init(ChiakiAudioReceiver *audio_receiver, ChiakiSession *session)
{
	audio_receiver->session = session;
	audio_receiver->log = &session->log;
	audio_receiver->opus_decoder = NULL;
	memset(&audio_receiver->audio_header, 0, sizeof(audio_receiver->audio_header));

	ChiakiErrorCode err = chiaki_mutex_init(&audio_receiver->mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	return CHIAKI_ERR_SUCCESS;
}


CHIAKI_EXPORT void chiaki_audio_receiver_fini(ChiakiAudioReceiver *audio_receiver)
{
	opus_decoder_destroy(audio_receiver->opus_decoder);
	chiaki_mutex_fini(&audio_receiver->mutex);
}


CHIAKI_EXPORT void chiaki_audio_receiver_stream_info(ChiakiAudioReceiver *audio_receiver, ChiakiAudioHeader *audio_header)
{
	chiaki_mutex_lock(&audio_receiver->mutex);

	CHIAKI_LOGI(audio_receiver->log, "Audio Header:\n");
	CHIAKI_LOGI(audio_receiver->log, "  channels = %d\n", audio_header->channels);
	CHIAKI_LOGI(audio_receiver->log, "  bits = %d\n", audio_header->bits);
	CHIAKI_LOGI(audio_receiver->log, "  rate = %d\n", audio_header->rate);
	CHIAKI_LOGI(audio_receiver->log, "  frame size = %d\n", audio_header->frame_size);
	CHIAKI_LOGI(audio_receiver->log, "  unknown = %d\n", audio_header->unknown);
	memcpy(&audio_receiver->audio_header, audio_header, sizeof(audio_receiver->audio_header));

	opus_decoder_destroy(audio_receiver->opus_decoder);

	int error;
	audio_receiver->opus_decoder = opus_decoder_create(audio_header->rate, audio_header->channels, &error);

	if(error != OPUS_OK)
		CHIAKI_LOGE(audio_receiver->log, "Audio Receiver failed to initialize opus decoder: %s\n", opus_strerror(error));
	else
		CHIAKI_LOGI(audio_receiver->log, "Audio Receiver initialized opus decoder with the settings above\n");

	chiaki_mutex_unlock(&audio_receiver->mutex);
}


CHIAKI_EXPORT void chiaki_audio_receiver_frame_packet(ChiakiAudioReceiver *audio_receiver, uint8_t *buf, size_t buf_size)
{
	chiaki_mutex_lock(&audio_receiver->mutex);

	if(!audio_receiver->opus_decoder)
	{
		CHIAKI_LOGE(audio_receiver->log, "Received audio frame, but opus decoder is not initialized\n");
		chiaki_mutex_unlock(&audio_receiver->mutex);
		return;
	}

	// TODO: don't malloc
	opus_int16 *pcm = malloc(audio_receiver->audio_header.frame_size * audio_receiver->audio_header.channels * sizeof(opus_int16));

	int r = opus_decode(audio_receiver->opus_decoder, buf, (opus_int32)buf_size, pcm, audio_receiver->audio_header.frame_size, 0);
	if(r < 1)
		CHIAKI_LOGE(audio_receiver->log, "Decoding audio frame with opus failed: %s\n", opus_strerror(r));
	else
	{
		audio_receiver->session->audio_frame_cb(pcm, (size_t)r, audio_receiver->session->audio_frame_cb_user);
	}

	free(pcm);

	chiaki_mutex_unlock(&audio_receiver->mutex);
}