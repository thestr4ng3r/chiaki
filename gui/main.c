#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

void audio_frame_cb(int8_t *buf, size_t samples_count, void *user)
{
	printf("AUDIO FRAME CB %lu\n", samples_count);
}

int main(int argc, const char *argv[])
{
	if(argc != 7)
	{
		printf("Usage: %s <host> <registkey> <ostype> <auth> <morning (base64)> <did>\n", argv[0]);
		return 1;
	}

	ChiakiConnectInfo connect_info;
	connect_info.host = argv[1];
	connect_info.regist_key = argv[2];
	connect_info.ostype = argv[3];

	size_t auth_len = strlen(argv[4]);
	if(auth_len > sizeof(connect_info.auth))
		auth_len = sizeof(connect_info.auth);
	memcpy(connect_info.auth, argv[4], auth_len);
	if(auth_len < sizeof(connect_info.auth))
		memset(connect_info.auth + auth_len, 0, sizeof(connect_info.auth) - auth_len);

	size_t morning_size = sizeof(connect_info.morning);
	ChiakiErrorCode err = chiaki_base64_decode(argv[5], strlen(argv[5]), connect_info.morning, &morning_size);
	if(err != CHIAKI_ERR_SUCCESS || morning_size != sizeof(connect_info.morning))
	{
		printf("morning invalid.\n");
		return 1;
	}

	size_t did_size = sizeof(connect_info.did);
	err = chiaki_base64_decode(argv[6], strlen(argv[6]), connect_info.did, &did_size);
	if(err != CHIAKI_ERR_SUCCESS || did_size != sizeof(connect_info.did))
	{
		printf("did invalid.\n");
		return 1;
	}

	ChiakiSession session;
	chiaki_session_init(&session, &connect_info);
	chiaki_session_set_audio_frame_cb(&session, audio_frame_cb, NULL);
	chiaki_session_start(&session);
	chiaki_session_join(&session);
	chiaki_session_fini(&session);
	return 0;
}