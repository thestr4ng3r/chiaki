#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <chiaki/pihwdecoder.h>

void chiaki_pihw_decoder_init(ChiakiPihwDecoder *decoder, ChiakiLog *log)
{

	decoder->video_decode = NULL;
	decoder->video_scheduler = NULL;
	decoder->video_render = NULL;

	bcm_host_init();

	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	COMPONENT_T *clock = NULL;

	memset(decoder->list, 0, sizeof(decoder->list));
	memset(decoder->tunnel, 0, sizeof(decoder->tunnel));

	if((decoder->client = ilclient_init()) == NULL) {
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to initialize video");
		return;
	}

	if(OMX_Init() != OMX_ErrorNone) {
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to initialize OMX");
		return;
	}

	// create video_decode
	if(ilclient_create_component(decoder->client, &decoder->video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
	{
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to create video decode");
		return;
	}

	decoder->list[0] = decoder->video_decode;

	// create video_render
	if(ilclient_create_component(decoder->client, &decoder->video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
	{
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to create video renderer");
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

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone ||
		OMX_SetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamBrcmDataUnit, &unit) != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to set video parameters");
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

	OMX_CONFIG_DISPLAYREGIONTYPE displayRegion;   // defined in OMX_Broadcom.h in vc
	displayRegion.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	displayRegion.nVersion.nVersion = OMX_VERSION;
	displayRegion.nPortIndex = 90;
	displayRegion.fullscreen = OMX_FALSE; //was: TRUE though doesn't seem to affect anything.
	displayRegion.mode = OMX_DISPLAY_MODE_FILL;

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigLatencyTarget, &latencyTarget) != OMX_ErrorNone)
	{
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to set video render parameters");
		return;
	}

	OMX_CONFIG_ROTATIONTYPE rotationType;
	memset(&rotationType, 0, sizeof(OMX_CONFIG_ROTATIONTYPE));
	rotationType.nSize = sizeof(OMX_CONFIG_ROTATIONTYPE);
	rotationType.nVersion.nVersion = OMX_VERSION;
	rotationType.nPortIndex = 90; 
	// These work if people ask for them later.
	/*
	int displayRotation = drFlags & DISPLAY_ROTATE_MASK;
	switch (displayRotation) {
		case DISPLAY_ROTATE_90:
		rotationType.nRotation = 90;
		break;
	case DISPLAY_ROTATE_180:
		rotationType.nRotation = 180;
		break;
	case DISPLAY_ROTATE_270:
		rotationType.nRotation = 270;
		break;
	}*/

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigCommonRotate, &rotationType) != OMX_ErrorNone) {
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to set video rotation");
		return;
	}

	OMX_PARAM_PORTDEFINITIONTYPE port;

	memset(&port, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	port.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	port.nVersion.nVersion = OMX_VERSION;
	port.nPortIndex = 130;
	if(OMX_GetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamPortDefinition, &port) != OMX_ErrorNone) {
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder failed to get decoder port definition");
		return;
	}

	// Increase the buffer size to fit the largest possible frame
	port.nBufferSize = MAX_DECODE_UNIT_SIZE;

	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamPortDefinition, &port) == OMX_ErrorNone &&
		ilclient_enable_port_buffers(decoder->video_decode, 130, NULL, NULL, NULL) == 0)
	{

	decoder->port_settings_changed = 0;
	decoder->first_packet = 1;

	ilclient_change_component_state(decoder->video_decode, OMX_StateExecuting);
	} else
	{
		CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder cannot set up video");
		return;
	}

	CHIAKI_LOGE(decoder->log, "ChiakiPihwDecoder Init successful");
}



void chiaki_pihw_decoder_fini(ChiakiPihwDecoder *decoder)
{
	free(decoder->pBuf);
}



void chiaki_pihw_decoder_get_buffer(ChiakiPihwDecoder *decoder, uint8_t *buffer, size_t buf_size)
{
	if(buf_size > 0 && buffer != NULL)
	{
		decoder->pBuf = buffer;
		decoder->_buf_size = buf_size;
	}
}

void chiaki_pihw_decoder_draw(ChiakiPihwDecoder *decoder)
{
	OMX_BUFFERHEADERTYPE *buf = NULL;
   
	while(decoder->pBuf != NULL)
	{
		if (buf == NULL)
		{
			if((buf = ilclient_get_input_buffer(decoder->video_decode, 130, 1)) == NULL)
			{
				fprintf(stderr, "ChiakiPihwDecoder can't get video buffer\n");
				return;
			}
		 
			buf->nFilledLen = 0;
			buf->nOffset = 0;
			buf->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS;
			if(decoder->first_packet)
			{
				buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
				decoder->first_packet = 0;
			}
		}
	

		memcpy(buf->pBuffer + buf->nFilledLen, decoder->pBuf, decoder->_buf_size);
		buf->nFilledLen += decoder->_buf_size;

		if(decoder->port_settings_changed == 0 && ((buf->nFilledLen > 0 && ilclient_remove_event(decoder->video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) || (buf->nFilledLen == 0 && ilclient_wait_for_event(decoder->video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1, ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
		{
			decoder->port_settings_changed = 1;

			if(ilclient_setup_tunnel(decoder->tunnel, 0, 0) != 0)
			{
				fprintf(stderr, "ChiakiPihwDecoder can't set up decoder tunnel\n");
				return;
			}

			ilclient_change_component_state(decoder->video_render, OMX_StateExecuting);
		}

	if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(decoder->video_decode), buf) != OMX_ErrorNone)
	{
		fprintf(stderr, "ChiakiPihwDecoder can't empty decoder buffer\n");
		return;
	}
	
	buf = NULL;
	decoder->pBuf=NULL;
	}

	return;
}
