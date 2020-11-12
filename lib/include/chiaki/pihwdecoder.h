#ifndef CHIAKI_PIHWDECODER_H
#define CHIAKI_PIHWDECODER_H

#include <chiaki/config.h>
#include <chiaki/log.h>

#include "bcm_host.h"
#include "OMX_Broadcom.h"
#include "ilclient.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*ChiakiPihwDecoderFrameCallback)(int16_t *buf, size_t samples_count, void *user);


typedef struct chiaki_pihw_decoder_t
{
	ChiakiLog *log;

	#define MAX_DECODE_UNIT_SIZE 262144

	TUNNEL_T     tunnel[2];
	COMPONENT_T *list[3];   // array of pointers
	ILCLIENT_T  *client;
	COMPONENT_T *video_decode;
	COMPONENT_T *video_scheduler;
	COMPONENT_T *video_render; 
	int port_settings_changed;
	int first_packet;
	uint8_t *pBuf;
	size_t  _buf_size;
	
	void *cb_user;
	ChiakiPihwDecoderFrameCallback frame_cb;
	
} ChiakiPihwDecoder;

CHIAKI_EXPORT void chiaki_pihw_decoder_init(ChiakiPihwDecoder *decoder, ChiakiLog *log);
CHIAKI_EXPORT void chiaki_pihw_decoder_fini(ChiakiPihwDecoder *decoder);
CHIAKI_EXPORT void chiaki_pihw_decoder_get_buffer(ChiakiPihwDecoder *decoder, uint8_t *buffer, size_t buf_size);
CHIAKI_EXPORT void chiaki_pihw_decoder_draw(ChiakiPihwDecoder *decoder);

static inline void chiaki_pihw_decoder_set_cb(ChiakiPihwDecoder *decoder, ChiakiPihwDecoderFrameCallback frame_cb, void *user)
{
	decoder->frame_cb = frame_cb;
	decoder->cb_user = user;
}

#ifdef __cplusplus
}
#endif 

#endif // CHIAKI_PIHWDECODER_H

