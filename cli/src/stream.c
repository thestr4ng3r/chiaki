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

#include <chiaki-cli.h>

#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <argp.h>
#include <string.h>
#include <unistd.h>

static char doc[] = "Request for PS4 stream.";

#define ARG_KEY_HOST 'h'
#define ARG_KEY_REGISTKEY 'r'
#define ARG_KEY_MORNING 'm'

static void audio_header_cb(ChiakiAudioHeader *header, void *user);
static void audio_frame_cb(uint8_t *buf, size_t buf_size, void *user);
static bool video_frame_cb(uint8_t *buf, size_t buf_size, void *user);

static struct argp_option options[] = {
	{ "host", ARG_KEY_HOST, "Host", 0, "Host to request stream from", 0 },
	{ "registkey", ARG_KEY_REGISTKEY, "RegistKey", 0, "PS4 registration key", 0 },
	{ "morning", ARG_KEY_MORNING, "Morning", 0, "PS4 morning encoded in base64", 0 },
	{ 0 }
};

typedef struct arguments
{
	const char *host;
	const char *registkey;
	const char *morning;
} Arguments;

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	Arguments *arguments = state->input;

	switch(key)
	{
		case ARG_KEY_HOST:
			arguments->host = arg;
			break;
		case ARG_KEY_REGISTKEY:
			arguments->registkey = arg;
			break;
		case ARG_KEY_MORNING:
			arguments->morning = arg;
			break;
		case ARGP_KEY_ARG:
			argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc, 0, 0, 0 };

static void audio_header_cb(ChiakiAudioHeader *header, void *user)
{
	uint8_t opus_id_head[0x13];
	memcpy(opus_id_head, "OpusHead", 8);
	opus_id_head[0x8] = 1; // version
	opus_id_head[0x9] = header->channels;
	uint16_t pre_skip = 3840;
	opus_id_head[0xa] = (uint8_t)(pre_skip & 0xff);
	opus_id_head[0xb] = (uint8_t)(pre_skip >> 8);
	opus_id_head[0xc] = (uint8_t)(header->rate & 0xff);
	opus_id_head[0xd] = (uint8_t)((header->rate >> 0x8) & 0xff);
	opus_id_head[0xe] = (uint8_t)((header->rate >> 0x10) & 0xff);
	opus_id_head[0xf] = (uint8_t)(header->rate >> 0x18);
	uint16_t output_gain = 0;
	opus_id_head[0x10] = (uint8_t)(output_gain & 0xff);
	opus_id_head[0x11] = (uint8_t)(output_gain >> 8);
	opus_id_head[0x12] = 0; // channel map
	audio_frame_cb(opus_id_head, sizeof(opus_id_head), user);

	uint64_t pre_skip_ns = 0;
	uint8_t csd1[8] = { (uint8_t)(pre_skip_ns & 0xff), (uint8_t)((pre_skip_ns >> 0x8) & 0xff), (uint8_t)((pre_skip_ns >> 0x10) & 0xff), (uint8_t)((pre_skip_ns >> 0x18) & 0xff),
						(uint8_t)((pre_skip_ns >> 0x20) & 0xff), (uint8_t)((pre_skip_ns >> 0x28) & 0xff), (uint8_t)((pre_skip_ns >> 0x30) & 0xff), (uint8_t)(pre_skip_ns >> 0x38)};
	audio_frame_cb(csd1, sizeof(csd1), user);

	uint64_t pre_roll_ns = 0;
	uint8_t csd2[8] = { (uint8_t)(pre_roll_ns & 0xff), (uint8_t)((pre_roll_ns >> 0x8) & 0xff), (uint8_t)((pre_roll_ns >> 0x10) & 0xff), (uint8_t)((pre_roll_ns >> 0x18) & 0xff),
						(uint8_t)((pre_roll_ns >> 0x20) & 0xff), (uint8_t)((pre_roll_ns >> 0x28) & 0xff), (uint8_t)((pre_roll_ns >> 0x30) & 0xff), (uint8_t)(pre_roll_ns >> 0x38)};
	audio_frame_cb(csd2, sizeof(csd2), user);
}

static void audio_frame_cb(uint8_t *buf, size_t buf_size, void *user)
{
	fwrite(buf, buf_size, 1, stderr);
}

static bool video_frame_cb(uint8_t *buf, size_t buf_size, void *user)
{
	fwrite(buf, buf_size, 1, stderr);
}

CHIAKI_EXPORT int chiaki_cli_cmd_stream(ChiakiLog *log, int argc, char *argv[])
{
	Arguments arguments = { 0 };
	error_t argp_r = argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &arguments);
	if(argp_r != 0)
		return 1;

	if(!arguments.host)
	{
		fprintf(stderr, "No host specified, see --help.\n");
		return 1;
	}
	if(!arguments.registkey)
	{
		fprintf(stderr, "No registration key specified, see --help.\n");
		return 1;
	}
	if(!arguments.morning)
	{
		fprintf(stderr, "No morning specified, see --help.\n");
		return 1;
	}
	if(strlen(arguments.registkey) > 8)
	{
		fprintf(stderr, "Given registkey is too long.\n");
		return 1;
	}

	ChiakiConnectInfo chiaki_connect_info;
	size_t morning_size = sizeof(chiaki_connect_info.morning);
	uint8_t morning[morning_size];
	chiaki_base64_decode(arguments.morning, strlen(arguments.morning), morning, &morning_size);
	memcpy(chiaki_connect_info.morning, morning, morning_size);
	chiaki_connect_info.host = arguments.host;
	memcpy(chiaki_connect_info.regist_key, arguments.registkey, sizeof(chiaki_connect_info.regist_key));

	ChiakiConnectVideoProfile video_profile;
	video_profile.width = 640;
	video_profile.height = 360;
	video_profile.max_fps = 30;
	video_profile.bitrate = 128;
	chiaki_connect_info.video_profile = video_profile;

	ChiakiSession session;
	ChiakiErrorCode err;
	err = chiaki_session_init(&session, &chiaki_connect_info, NULL);
	if(err != CHIAKI_ERR_SUCCESS)
		return 2;

	ChiakiAudioSink audio_sink;
	audio_sink.header_cb = audio_header_cb;
	audio_sink.frame_cb = audio_frame_cb;
	chiaki_session_set_audio_sink(&session, &audio_sink);

	// Video seems to work fine for now, commenting this so it doesn't
	// interfere with my attempts to get audio working.
	// chiaki_session_set_video_sample_cb(&session, video_frame_cb, NULL);

	err = chiaki_session_start(&session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_session_fini(&session);
		return 2;
	}

	// For testing purpose. Quit after 20 secs.
	sleep(20);
	chiaki_session_fini(&session);
	return 0;
}
