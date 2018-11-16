#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	if(argc != 6)
	{
		printf("Usage: %s <host> <registkey> <ostype> <auth> <morning (base64)>\n", argv[0]);
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

	ChiakiSession session;
	chiaki_session_init(&session, &connect_info);
	chiaki_session_start(&session);
	chiaki_session_join(&session);
	chiaki_session_fini(&session);
	return 0;
}