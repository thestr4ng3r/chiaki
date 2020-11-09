// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include <videodecoder.h>

#include <libavcodec/avcodec.h>

#include <QImage>

VideoDecoder::VideoDecoder(HardwareDecodeEngine hw_decode_engine, ChiakiLog *log) : hw_decode_engine(hw_decode_engine), log(log)
{
	enum AVHWDeviceType type;
	hw_device_ctx = nullptr;

	#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)
	avcodec_register_all();
	#endif
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(!codec)
		throw VideoDecoderException("H264 Codec not available");

	codec_context = avcodec_alloc_context3(codec);
	if(!codec_context)
		throw VideoDecoderException("Failed to alloc codec context");

	if(hw_decode_engine)
	{
		if(!hardware_decode_engine_names.contains(hw_decode_engine))
			throw VideoDecoderException("Unknown hardware decode engine!");

		const char *hw_dec_eng = hardware_decode_engine_names[hw_decode_engine];
		CHIAKI_LOGI(log, "Using hardware decode %s", hw_dec_eng);
		type = av_hwdevice_find_type_by_name(hw_dec_eng);
		if (type == AV_HWDEVICE_TYPE_NONE)
			throw VideoDecoderException("Can't initialize vaapi");

		for(int i = 0;; i++) {
			const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
			if(!config)
				throw VideoDecoderException("avcodec_get_hw_config failed");
			if(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
				config->device_type == type)
			{
				hw_pix_fmt = config->pix_fmt;
				break;
			}
		}

		if(av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0) < 0)
			throw VideoDecoderException("Failed to create hwdevice context");
		codec_context->hw_device_ctx = av_buffer_ref(hw_device_ctx);
	}

	if(avcodec_open2(codec_context, codec, nullptr) < 0)
	{
		avcodec_free_context(&codec_context);
		throw VideoDecoderException("Failed to open codec context");
	}
}

VideoDecoder::~VideoDecoder()
{
	avcodec_close(codec_context);
	avcodec_free_context(&codec_context);
	if(hw_device_ctx)
	{
		av_buffer_unref(&hw_device_ctx);
	}
}

void VideoDecoder::PushFrame(uint8_t *buf, size_t buf_size)
{
	{
		QMutexLocker locker(&mutex);

		AVPacket packet;
		av_init_packet(&packet);
		packet.data = buf;
		packet.size = buf_size;
		int r;
send_packet:
		r = avcodec_send_packet(codec_context, &packet);
		if(r != 0)
		{
			if(r == AVERROR(EAGAIN))
			{
				CHIAKI_LOGE(log, "AVCodec internal buffer is full removing frames before pushing");
				AVFrame *frame = av_frame_alloc();
				if(!frame)
				{
					CHIAKI_LOGE(log, "Failed to alloc AVFrame");
					return;
				}
				r = avcodec_receive_frame(codec_context, frame);
				av_frame_free(&frame);
				if(r != 0)
				{
					CHIAKI_LOGE(log, "Failed to pull frame");
					return;
				}
				goto send_packet;
			}
			else
			{
				char errbuf[128];
				av_make_error_string(errbuf, sizeof(errbuf), r);
				CHIAKI_LOGE(log, "Failed to push frame: %s", errbuf);
				return;
			}
		}
	}

	emit FramesAvailable();
}

AVFrame *VideoDecoder::PullFrame()
{
	QMutexLocker locker(&mutex);

	// always try to pull as much as possible and return only the very last frame
	AVFrame *frame_last = nullptr;
	AVFrame *sw_frame = nullptr;
	AVFrame *frame = nullptr; 
	while(true)
	{
		AVFrame *next_frame;
		if(frame_last)
		{
			av_frame_unref(frame_last);
			next_frame = frame_last;
		}
		else
		{
			next_frame = av_frame_alloc();
			if(!next_frame)
				return frame;
		}
		frame_last = frame;
		frame = next_frame;
		int r = avcodec_receive_frame(codec_context, frame);
		if(r == 0)
		{
			frame = hw_decode_engine ? GetFromHardware(frame) : frame;
		}
		else
		{
			if(r != AVERROR(EAGAIN))
				CHIAKI_LOGE(log, "Decoding with FFMPEG failed");
			av_frame_free(&frame);
			return frame_last;
		}
	}
}

AVFrame *VideoDecoder::GetFromHardware(AVFrame *hw_frame)
{
	AVFrame *frame;
	AVFrame *sw_frame;

	sw_frame = av_frame_alloc();

	int ret = av_hwframe_transfer_data(sw_frame, hw_frame, 0);

	if(ret < 0)
	{
		CHIAKI_LOGE(log, "Failed to transfer frame from hardware");
	}

	av_frame_unref(hw_frame);

	if(sw_frame->width <= 0)
	{
		av_frame_unref(sw_frame);
		return nullptr;
	}

	return sw_frame;
}
