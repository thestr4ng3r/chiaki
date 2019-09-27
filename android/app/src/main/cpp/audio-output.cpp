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

#include "audio-output.h"

#include <chiaki/log.h>
#include <chiaki/thread.h>

#include <oboe/Oboe.h>

#define BUFFER_SIZE_DEFAULT 409600

class AudioOutput;

class AudioOutputCallback: public oboe::AudioStreamCallback
{
private:
	AudioOutput *audio_output;

public:
	AudioOutputCallback(AudioOutput *audio_output) : audio_output(audio_output) {}
	oboe::DataCallbackResult onAudioReady(oboe::AudioStream *stream, void *audioData, int32_t numFrames) override;
	void onErrorBeforeClose(oboe::AudioStream *stream, oboe::Result error) override;
	void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) override;
};

struct AudioOutput
{
	ChiakiLog *log;
	oboe::ManagedStream stream;
	AudioOutputCallback stream_callback;

	ChiakiMutex buffer_mutex; // TODO: make lockless
	uint8_t *buffer;
	size_t buffer_size; // total size
	size_t buffer_start;
	size_t buffer_length; // filled size

	AudioOutput() : stream_callback(this) {}
};

extern "C" void *android_chiaki_audio_output_new(ChiakiLog *log)
{
	auto r = new AudioOutput();
	r->log = log;

	if(chiaki_mutex_init(&r->buffer_mutex, false) != CHIAKI_ERR_SUCCESS)
	{
		delete r;
		return nullptr;
	}
	r->buffer_size = BUFFER_SIZE_DEFAULT;
	r->buffer = (uint8_t *)malloc(r->buffer_size);
	if(!r->buffer)
	{
		chiaki_mutex_fini(&r->buffer_mutex);
		delete r;
		return nullptr;
	}
	r->buffer_start = 0;
	r->buffer_length = 0;

	return r;
}

extern "C" void android_chiaki_audio_output_free(void *audio_output)
{
	if(!audio_output)
		return;
	auto ao = reinterpret_cast<AudioOutput *>(audio_output);
	ao->stream = nullptr;
	free(ao->buffer);
	chiaki_mutex_fini(&ao->buffer_mutex);
	delete ao;
}

extern "C" void android_chiaki_audio_output_settings(uint32_t channels, uint32_t rate, void *audio_output)
{
	auto ao = reinterpret_cast<AudioOutput *>(audio_output);

	oboe::AudioStreamBuilder builder;
	builder.setPerformanceMode(oboe::PerformanceMode::LowLatency)
		->setSharingMode(oboe::SharingMode::Exclusive)
		->setFormat(oboe::AudioFormat::I16)
		->setChannelCount(channels)
		->setSampleRate(rate)
		->setCallback(&ao->stream_callback);

	auto result = builder.openManagedStream(ao->stream);
	if(result == oboe::Result::OK)
		CHIAKI_LOGI(ao->log, "Audio Output opened Oboe stream");
	else
		CHIAKI_LOGE(ao->log, "Audio Output failed to open Oboe stream: %s", oboe::convertToText(result));

	result = ao->stream->start();
	if(result == oboe::Result::OK)
		CHIAKI_LOGI(ao->log, "Audio Output started Oboe stream");
	else
		CHIAKI_LOGE(ao->log, "Audio Output failed to start Oboe stream: %s", oboe::convertToText(result));
}

extern "C" void android_chiaki_audio_output_frame(int16_t *buf, size_t samples_count, void *audio_output)
{
	auto ao = reinterpret_cast<AudioOutput *>(audio_output);
	ChiakiMutexLock lock(&ao->buffer_mutex);

	size_t buf_size = samples_count * sizeof(int16_t);
	if(ao->buffer_size - ao->buffer_length < buf_size)
	{
		CHIAKI_LOGW(ao->log, "Audio Output Buffer Overflow!");
		buf_size = ao->buffer_size - ao->buffer_length;
	}

	if(!buf_size)
		return;

	size_t end = (ao->buffer_start + ao->buffer_length) % ao->buffer_size;
	ao->buffer_length += buf_size;
	if(end + buf_size > ao->buffer_size)
	{
		size_t part = ao->buffer_size - end;
		memcpy(ao->buffer + end, buf, part);
		buf_size -= part;
		buf = (int16_t *)(((uint8_t *)buf) + part);
		end = 0;
	}
	memcpy(ao->buffer + end, buf, buf_size);
}

oboe::DataCallbackResult AudioOutputCallback::onAudioReady(oboe::AudioStream *stream, void *audio_data, int32_t num_frames)
{
	if(stream->getFormat() != oboe::AudioFormat::I16)
	{
		CHIAKI_LOGE(audio_output->log, "Oboe stream has invalid format in callback");
		return oboe::DataCallbackResult::Stop;
	}

	int32_t bytes_per_frame = stream->getBytesPerFrame();
	size_t buf_size_requested = static_cast<size_t>(bytes_per_frame * num_frames);

	size_t buf_size_delivered = buf_size_requested;
	uint8_t *buf = (uint8_t *)audio_data;

	{
		ChiakiMutexLock lock(&audio_output->buffer_mutex);
		if(audio_output->buffer_length < buf_size_delivered)
		{
			CHIAKI_LOGW(audio_output->log, "Audio Output Buffer Underflow!");
			buf_size_delivered = audio_output->buffer_length;
		}

		if(audio_output->buffer_start + buf_size_delivered > audio_output->buffer_size)
		{
			size_t part = audio_output->buffer_size - audio_output->buffer_start;
			memcpy(buf, audio_output->buffer + audio_output->buffer_start, part);
			memcpy(buf + part, audio_output->buffer, buf_size_delivered - part);
		}
		else
			memcpy(buf, audio_output->buffer + audio_output->buffer_start, buf_size_delivered);

		audio_output->buffer_start = (audio_output->buffer_start + buf_size_delivered) % audio_output->buffer_size;
		audio_output->buffer_length -= buf_size_delivered;
	}

	if(buf_size_delivered < buf_size_requested)
		memset(buf + buf_size_delivered, 0, buf_size_requested - buf_size_delivered);

	return oboe::DataCallbackResult::Continue;
}

void AudioOutputCallback::onErrorBeforeClose(oboe::AudioStream *stream, oboe::Result error)
{
	CHIAKI_LOGE(audio_output->log, "Oboe reported error before close: %s", oboe::convertToText(error));
}

void AudioOutputCallback::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error)
{
	CHIAKI_LOGE(audio_output->log, "Oboe reported error after close: %s", oboe::convertToText(error));
}
