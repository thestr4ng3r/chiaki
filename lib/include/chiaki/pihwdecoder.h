#ifndef CHIAKI_PIHWDECODER_H
#define CHIAKI_PIHWDECODER_H

#include <chiaki/config.h>
#include <chiaki/log.h>

#include <ilclient.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP


typedef void (*ChiakiPihwDecoderFrameCallback)(int16_t *buf, size_t samples_count, void *user);

typedef struct chiaki_pihw_decoder_t
{
	ChiakiLog *log;

	#define MAX_DECODE_UNIT_SIZE 262144

	TUNNEL_T     tunnel[2];
	COMPONENT_T *list[4];   // array of pointers, was[3] but added GL for [4]
	ILCLIENT_T  *client;
	COMPONENT_T *video_decode;
	COMPONENT_T *video_scheduler;
	COMPONENT_T *video_render;
	int port_settings_changed;
	int first_packet;
	uint8_t *pBuf;
	size_t  _buf_size;

	ChiakiPihwDecoderFrameCallback frame_cb;
	void *cb_user;
	
	// Enable for video save out.
	//FILE *outf;
	
} ChiakiPihwDecoder;

CHIAKI_EXPORT void chiaki_pihw_decoder_init(ChiakiPihwDecoder *decoder, ChiakiLog *log);
CHIAKI_EXPORT void chiaki_pihw_decoder_fini(ChiakiPihwDecoder *decoder);
CHIAKI_EXPORT void chiaki_pihw_decoder_get_buffer(ChiakiPihwDecoder *decoder, uint8_t *buffer, size_t buf_size);
CHIAKI_EXPORT void chiaki_pihw_decoder_draw(ChiakiPihwDecoder *decoder);

CHIAKI_EXPORT void chiaki_pihw_decoder_transform(ChiakiPihwDecoder *decoder, int x, int y, int w, int h);
CHIAKI_EXPORT void chiaki_pihw_decoder_visibility(ChiakiPihwDecoder *decoder, int vis); // 0 or 1

static inline void chiaki_pihw_decoder_set_cb(ChiakiPihwDecoder *decoder, ChiakiPihwDecoderFrameCallback frame_cb, void *user)
{
	decoder->frame_cb = frame_cb;
	decoder->cb_user = user;
}


#ifdef __cplusplus
}
#endif 

#endif // CHIAKI_PIHWDECODER_H
