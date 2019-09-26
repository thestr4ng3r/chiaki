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

#define WRITE_TIMEOUT_MS 8

struct AudioOutput
{
	ChiakiLog *log;
	oboe::ManagedStream stream;
	ChiakiMutex stream_mutex;
};

extern "C" void *android_chiaki_audio_output_new(ChiakiLog *log)
{
	auto r = new AudioOutput();
	r->log = log;
	chiaki_mutex_init(&r->stream_mutex, false);
	return r;
}

extern "C" void android_chiaki_audio_output_free(void *audio_output)
{
	if(!audio_output)
		return;
	auto ao = reinterpret_cast<AudioOutput *>(audio_output);
	chiaki_mutex_fini(&ao->stream_mutex);
	delete ao;
}

extern "C" void android_chiaki_audio_output_settings(uint32_t channels, uint32_t rate, void *audio_output)
{
	auto ao = reinterpret_cast<AudioOutput *>(audio_output);
	ChiakiMutexLock lock(&ao->stream_mutex);

	oboe::AudioStreamBuilder builder;
	builder.setPerformanceMode(oboe::PerformanceMode::LowLatency)
		->setSharingMode(oboe::SharingMode::Exclusive)
		->setFormat(oboe::AudioFormat::I16)
		->setChannelCount(channels)
		->setSampleRate(rate);

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
	ChiakiMutexLock lock(&ao->stream_mutex);

	if(ao->stream->getFormat() != oboe::AudioFormat::I16)
	{
		CHIAKI_LOGE(ao->log, "Oboe stream has invalid format for pushing samples");
		return;
	}

	int32_t samples_per_frame = ao->stream->getBytesPerFrame() / ao->stream->getBytesPerSample();
	if(samples_count % samples_per_frame != 0)
		CHIAKI_LOGW(ao->log, "Received %llu samples, which is not dividable by %llu samples per frame", (unsigned long long)samples_count, (unsigned long long)samples_per_frame);

	size_t frames = samples_count / samples_per_frame;
	if(frames > 0)
		ao->stream->write(buf, frames, WRITE_TIMEOUT_MS * 1000 * 1000);
}