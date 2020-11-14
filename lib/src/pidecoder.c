
#include <chiaki/pidecoder.h>

#include <bcm_host.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_DECODE_UNIT_SIZE 262144

CHIAKI_EXPORT void chiaki_pi_decoder_init(ChiakiPiDecoder *decoder, ChiakiLog *log)
{
	decoder->video_decode = NULL;
	decoder->video_scheduler = NULL;
	decoder->video_render = NULL;
	
	bcm_host_init();

	OMX_VIDEO_PARAM_PORTFORMATTYPE format;

	memset(decoder->list, 0, sizeof(decoder->list));    //array of pointers
	memset(decoder->tunnel, 0, sizeof(decoder->tunnel)); //array  

	if(!(decoder->client = ilclient_init()))
	{
		CHIAKI_LOGE(decoder->log, "ilclient_init failed");
		return;
	}

	if(OMX_Init() != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "OMX_Init failed");
		// TODO: leak here
		return;
	}

	if(ilclient_create_component(decoder->client, &decoder->video_decode, "video_decode",
			ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
	{
		CHIAKI_LOGE(decoder->log, "ilclient_create_component failed for video_decode");
		// TODO: leak here
		return;
	}

	decoder->list[0] = decoder->video_decode;

	if(ilclient_create_component(decoder->client, &decoder->video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
	{
		CHIAKI_LOGE(decoder->log, "ilclient_create_component failed for video_render");
		// TODO: leak here
		return;
	}

	decoder->list[1] = decoder->video_render;

	set_tunnel(decoder->tunnel, decoder->video_decode, 131, decoder->video_render, 90);

	ilclient_change_component_state(decoder->video_decode, OMX_StateIdle);

	memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	format.nVersion.nVersion = OMX_VERSION;
	format.nPortIndex = 130;
	format.eCompressionFormat = OMX_VIDEO_CodingAVC;

	OMX_PARAM_DATAUNITTYPE unit;

	memset(&unit, 0, sizeof(OMX_PARAM_DATAUNITTYPE));
	unit.nSize = sizeof(OMX_PARAM_DATAUNITTYPE);
	unit.nVersion.nVersion = OMX_VERSION;
	unit.nPortIndex = 130;
	unit.eUnitType = OMX_DataUnitCodedPicture;
	unit.eEncapsulationType = OMX_DataEncapsulationElementaryStream;

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone
		|| OMX_SetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamBrcmDataUnit, &unit) != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "OMX_SetParameter failed for video parameters");
		// TODO: leak here
		return;
	}

	OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
	memset(&latencyTarget, 0, sizeof(OMX_CONFIG_LATENCYTARGETTYPE));
	latencyTarget.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	latencyTarget.nVersion.nVersion = OMX_VERSION;
	latencyTarget.nPortIndex = 90;
	latencyTarget.bEnabled = OMX_TRUE;
	latencyTarget.nFilter = 2;
	latencyTarget.nTarget = 4000;
	latencyTarget.nShift = 3;
	latencyTarget.nSpeedFactor = -135;
	latencyTarget.nInterFactor = 500;
	latencyTarget.nAdjCap = 20;

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigLatencyTarget, &latencyTarget) != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "OMX_SetParameter failed for render parameters");
		// TODO: leak here
		// TODO: leak here
		return;
	}

	OMX_CONFIG_ROTATIONTYPE rotationType;
	memset(&rotationType, 0, sizeof(OMX_CONFIG_ROTATIONTYPE));
	rotationType.nSize = sizeof(OMX_CONFIG_ROTATIONTYPE);
	rotationType.nVersion.nVersion = OMX_VERSION;
	rotationType.nPortIndex = 90;
	//rotationType.nRotation = 90; // example

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigCommonRotate, &rotationType) != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "OMX_SetParameter failed for rotation");
		// TODO: leak here
		return;
	}

	OMX_PARAM_PORTDEFINITIONTYPE port;

	memset(&port, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	port.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	port.nVersion.nVersion = OMX_VERSION;
	port.nPortIndex = 130;
	if(OMX_GetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamPortDefinition, &port) != OMX_ErrorNone)
	{
		printf("Failed to get decoder port definition\n");
		// TODO: leak here
		return;
	}

	// Increase the buffer size to fit the largest possible frame
	port.nBufferSize = MAX_DECODE_UNIT_SIZE;

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamPortDefinition, &port) == OMX_ErrorNone &&
		ilclient_enable_port_buffers(decoder->video_decode, 130, NULL, NULL, NULL) == 0)
	{
		// TODO: make this flat and early return
		decoder->port_settings_changed = 0;
		decoder->first_packet = 1;

		ilclient_change_component_state(decoder->video_decode, OMX_StateExecuting);
	}
	else
	{
		printf("Failed to get decoder port definition\n");
		// TODO: leak here
		return;
	}

	CHIAKI_LOGI(decoder->log, "Raspberry Pi Decoder initialized");
}

CHIAKI_EXPORT void chiaki_pi_decoder_fini(ChiakiPiDecoder *decoder)
{
	OMX_BUFFERHEADERTYPE *buf;
	if((buf = ilclient_get_input_buffer(decoder->video_decode, 130, 1)) == NULL)
	{
		//printf("Can't get video buffer\n");
		return;
	}

	buf->nFilledLen = 0;
	buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

	if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(decoder->list[0]), buf) != OMX_ErrorNone)
	{
		//printf("Can't empty video buffer\n");
		return;
	}

	// need to flush the renderer to allow video_decode to disable its input port
	ilclient_flush_tunnels(decoder->tunnel, 0);

	ilclient_disable_port_buffers(decoder->list[0], 130, NULL, NULL, NULL);

	ilclient_disable_tunnel(decoder->tunnel);
	ilclient_teardown_tunnels(decoder->tunnel);

	ilclient_state_transition(decoder->list, OMX_StateIdle);
	ilclient_state_transition(decoder->list, OMX_StateLoaded);

	ilclient_cleanup_components(decoder->list);

	OMX_Deinit();
	ilclient_destroy(decoder->client);
}

static bool push_buffer(ChiakiPiDecoder *decoder, uint8_t *buf, size_t buf_size)
{
	OMX_BUFFERHEADERTYPE *omx_buf = ilclient_get_input_buffer(decoder->video_decode, 130, 1);
	if(!omx_buf)
	{
		fprintf(stderr, "Can't get video buffer\n");
		return false;
	}
	
	omx_buf->nFilledLen = 0;
	omx_buf->nOffset = 0;
	omx_buf->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS;
	if(decoder->first_packet)
	{
		omx_buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
		decoder->first_packet = 0;
	}

	memcpy(omx_buf->pBuffer + omx_buf->nFilledLen, buf, buf_size);
	omx_buf->nFilledLen += buf_size;

	if(decoder->port_settings_changed == 0
		&& ((omx_buf->nFilledLen > 0 && ilclient_remove_event(decoder->video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0)
		       	|| (omx_buf->nFilledLen == 0 && ilclient_wait_for_event(decoder->video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1, ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
	{
		decoder->port_settings_changed = 1;
	
		if(ilclient_setup_tunnel(decoder->tunnel, 0, 0) != 0)
		{
			CHIAKI_LOGE(decoder->log, "ilclient_setup_tunnel failed");
			return false;
		}
		
		ilclient_change_component_state(decoder->video_render, OMX_StateExecuting);
	}

	if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(decoder->video_decode), omx_buf) != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "OMX_EmptyThisBuffer failed");
		return false;
	}
	return true;
}

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP

CHIAKI_EXPORT void chiaki_pi_decoder_transform(ChiakiPiDecoder *decoder, int x, int y, int w, int h)
{	
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex = 90;
	configDisplay.set        = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_PIXEL | OMX_DISPLAY_SET_DEST_RECT | OMX_DISPLAY_SET_ALPHA);
	configDisplay.mode = OMX_DISPLAY_MODE_LETTERBOX;
	configDisplay.fullscreen = OMX_FALSE;
	configDisplay.noaspect   = OMX_FALSE;
	configDisplay.dest_rect.x_offset  = x;
	configDisplay.dest_rect.y_offset  = y;
	configDisplay.dest_rect.width     = w;
	configDisplay.dest_rect.height    = h;
	configDisplay.alpha = 255;

  	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
		CHIAKI_LOGE(decoder->log, "OMX_SetParameter failed for display region");
}

CHIAKI_EXPORT void chiaki_pi_decoder_visibility(ChiakiPiDecoder *decoder, bool visible)
{
	int opacity = visible ? 255 : 0; 
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex = 90;
	configDisplay.set        = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_PIXEL | OMX_DISPLAY_SET_ALPHA);
	configDisplay.mode = OMX_DISPLAY_MODE_LETTERBOX;
	configDisplay.fullscreen = OMX_FALSE;
	configDisplay.noaspect   = OMX_FALSE;
	configDisplay.alpha = opacity;

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
		CHIAKI_LOGE(decoder->log, "OMX_SetParameter failed for visibility");
}

CHIAKI_EXPORT bool chiaki_pi_decoder_video_sample_cb(uint8_t *buf, size_t buf_size, void *user)
{
	return push_buffer(user, buf, buf_size);
}
