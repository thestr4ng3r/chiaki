
#include <chiaki/pihwdecoder.h>

#include <bcm_host.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


void chiaki_pihw_decoder_init(ChiakiPihwDecoder *decoder, ChiakiLog *log)
{
			
	decoder->video_decode = NULL;
	decoder->video_scheduler = NULL;
	decoder->video_render = NULL;
	
	bcm_host_init();

	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	COMPONENT_T *clock = NULL;

	memset(decoder->list, 0, sizeof(decoder->list));    //array of pointers
	memset(decoder->tunnel, 0, sizeof(decoder->tunnel)); //array  

	if((decoder->client = ilclient_init()) == NULL) {
		fprintf(stderr, "Can't initialize video\n");
		return;
	}

	if(OMX_Init() != OMX_ErrorNone) {
		fprintf(stderr, "Can't initialize OMX\n");
		return;
	}

	// create video_decode
	if(ilclient_create_component(decoder->client, &decoder->video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
	{
		fprintf(stderr, "Can't create video decode\n");
		return;
	}

	decoder->list[0] = decoder->video_decode;

	// create video_render
	if(ilclient_create_component(decoder->client, &decoder->video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
	{
		fprintf(stderr, "Can't create video renderer\n");
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
		fprintf(stderr, "Failed to set video parameters\n");
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
		fprintf(stderr, "Failed to set video render parameters\n");
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
		fprintf(stderr, "Failed to set video rotation\n");
		return;
	}

	OMX_PARAM_PORTDEFINITIONTYPE port;

	memset(&port, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	port.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	port.nVersion.nVersion = OMX_VERSION;
	port.nPortIndex = 130;
	if(OMX_GetParameter(ILC_GET_HANDLE(decoder->video_decode), OMX_IndexParamPortDefinition, &port) != OMX_ErrorNone) {
		printf("Failed to get decoder port definition\n");
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
		fprintf(stderr, "Can't setup video\n");
		return;
	}
	
	// For video saveout
	//decoder->outf = fopen("chiaki_video_dump.h264", "w"); // Need to check this!
	

	printf("[I] RPi hardware setup successful\n");
}



void chiaki_pihw_decoder_fini(ChiakiPihwDecoder *decoder)
{
	
	OMX_BUFFERHEADERTYPE *buf;
	if((buf = ilclient_get_input_buffer(decoder->video_decode, 130, 1)) == NULL){
		//printf("Can't get video buffer\n");
		return;
	}

	buf->nFilledLen = 0;
	buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

	if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(decoder->list[0]), buf) != OMX_ErrorNone){
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
            fprintf(stderr, "Can't get video buffer\n");
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
			printf("ERR decodeFrameData:  A\n");
			return;
		}
		
		ilclient_change_component_state(decoder->video_render, OMX_StateExecuting);
	}



	if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(decoder->video_decode), buf) != OMX_ErrorNone)
	{
		printf("Can't empty video buffer\n");
		return;
	}
	
	
	// Saves h264 video dump.
	if(0)
	{
		//int r = 0;
		//r = fwrite(decoder->pBuf, 1, decoder->_buf_size, decoder->outf);
		// commandline to convert to mp4:
		// ffmpeg -framerate 60000/1001 -i chiaki_video_dump.h264 -c copy output.mp4
		// edit length doesn't work because lack of keyframes? Use Premiere.
	}
	

	buf = NULL;
	decoder->pBuf=NULL;
	}

   return;
}


CHIAKI_EXPORT void chiaki_pihw_decoder_transform(ChiakiPihwDecoder *decoder, int x, int y, int w, int h)
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
	{
		printf(stderr, "Failed to set DISPLAY REGION\n");
		return;
	}
	
}

CHIAKI_EXPORT void chiaki_pihw_decoder_visibility(ChiakiPihwDecoder *decoder, int vis)
{
	
	int opacity=255; 
	if(vis == 0)
		opacity=0;
	
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex = 90;
	configDisplay.set        = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_PIXEL | OMX_DISPLAY_SET_ALPHA);
	configDisplay.mode = OMX_DISPLAY_MODE_LETTERBOX;
	configDisplay.fullscreen = OMX_FALSE;
	configDisplay.noaspect   = OMX_FALSE;
	configDisplay.alpha = opacity;
	
	
	if(OMX_SetParameter(ILC_GET_HANDLE(decoder->video_render), OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
	{
		printf(stderr, "Failed to set DISPLAY REGION\n");
		return;
	}
}
