#include "D3DDecoder.h"



extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}


#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")





D3DPlayer::D3DDecoder::D3DDecoder(bool UseD3D9)
	: m_pDecoder(nullptr)
	, m_pDecoderContext(nullptr)
	, m_pHwDeviceContext(nullptr)
	, m_pOutputFrame(nullptr)

	, m_CodecID(AV_CODEC_ID_NONE)
	, m_HwDeviceType(UseD3D9 ? AV_HWDEVICE_TYPE_DXVA2 : AV_HWDEVICE_TYPE_D3D11VA)
	, m_HwPixelFormat(UseD3D9 ? AV_PIX_FMT_DXVA2_VLD : AV_PIX_FMT_D3D11)

	, m_Videoindex(-1)

	, m_VideoWidth(0)
	, m_VideoHeight(0)

	, m_VideoFramerate(0)

	, InitFailed(false)
{
}


D3DPlayer::D3DDecoder::~D3DDecoder()
{
	Deinitialize();
}


void D3DPlayer::D3DDecoder::Initialize(enum AVCodecID CodecID)
{
	if (m_pDecoder != nullptr) {
		return;
	}

	TRACEA(LOG_LEVEL_INFO, "<begin>");

	std::lock_guard<std::mutex> locker(m_mutexInitUninit);

	InitFailed = false;

	m_CodecID = CodecID;

	int ret = 0;
	BREAK_ENTER;
	ret = av_hwdevice_ctx_create(&m_pHwDeviceContext, m_HwDeviceType, "auto", NULL, 0);
	BREAK_FAIL(ret == 0, L"av_hwdevice_ctx_create");

	m_pDecoder = (AVCodec *)avcodec_find_decoder(m_CodecID);
	BREAK_FAIL(m_pDecoder != NULL, L"avcodec_find_decoder");

	for (int i = 0;; i++) {
		const AVCodecHWConfig *config = avcodec_get_hw_config(m_pDecoder, i);
		if (config == nullptr) {
			TRACEA(LOG_LEVEL_WARNING, "avcodec_get_hw_config, decoder does not support device type(%s)", av_hwdevice_get_type_name(m_HwDeviceType));
			break;
		}

		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == m_HwDeviceType) {
			if (config->pix_fmt != m_HwPixelFormat) {
				TRACEA(LOG_LEVEL_WARNING, "reset m_HwDeviceType(%s) with config->pix_fmt(%s)", av_get_pix_fmt_name(m_HwPixelFormat), av_get_pix_fmt_name(config->pix_fmt));
				m_HwPixelFormat = config->pix_fmt;
			}
			break;
		}
	}

	m_pDecoderContext = avcodec_alloc_context3(m_pDecoder);
	BREAK_FAIL(m_pDecoderContext != NULL, L"avcodec_alloc_context3");
	m_pDecoderContext->opaque = this;
	m_pDecoderContext->hw_device_ctx = av_buffer_ref(m_pHwDeviceContext);
	m_pDecoderContext->get_format = GetHwSurfaceFormat;

	ret = avcodec_open2(m_pDecoderContext, m_pDecoder, NULL);
	BREAK_FAIL(ret == 0, L"avcodec_open2");

	m_VideoWidth = m_pDecoderContext->width;
	m_VideoHeight = m_pDecoderContext->height;

	m_pOutputFrame = av_frame_alloc();
	BREAK_LEAVE;
	BREAK_CLEAN;

	InitFailed = !succeed;

	TRACEA(LOG_LEVEL_INFO, "</end>");
}


void D3DPlayer::D3DDecoder::Deinitialize()
{
	TRACEA(LOG_LEVEL_INFO, "<begin>");

	std::lock_guard<std::mutex> locker(m_mutexInitUninit);

	if (m_pOutputFrame != nullptr) {
		av_frame_free(&m_pOutputFrame);
		m_pOutputFrame = nullptr;
	}

	if (m_pDecoderContext != nullptr) {
		av_buffer_unref(&m_pHwDeviceContext);

		avcodec_free_context(&m_pDecoderContext);
		m_pDecoderContext = nullptr;
	}

	m_pDecoder = nullptr;

	m_CodecID = AV_CODEC_ID_NONE;

	TRACEA(LOG_LEVEL_INFO, "</end>");
}


int D3DPlayer::D3DDecoder::SendPacket(uint8_t *pBuffer, int size, int64_t dts, int64_t pts)
{
	AVPacket packet;
	if (av_new_packet(&packet, GetPacketPayloadSize()) != 0) {
		return -1;
	}

	packet.size = size;
	packet.dts = dts;
	packet.pts = pts;
	memcpy(packet.data, pBuffer, size);
	int ret = avcodec_send_packet(m_pDecoderContext, &packet);
	if (ret < 0) {
		TRACEA(LOG_LEVEL_WARNING, "Error during decoding with code 0x%x", ret);
	}

	av_packet_unref(&packet);

	return ret;
}


AVFrame *D3DPlayer::D3DDecoder::DecodeFrame(int &ret, int &keyFrame, uint64_t &dts, uint64_t &pts, int &width, int &height)
{
	ret = 0;

	while (ret >= 0) {
		ret = avcodec_receive_frame(m_pDecoderContext, m_pOutputFrame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			av_frame_unref(m_pOutputFrame);
			break;
		}
		else if (ret < 0) {
			TRACEA(LOG_LEVEL_WARNING, "Error while decoding with code 0x%x", ret);
			av_frame_unref(m_pOutputFrame);
			return nullptr;
		}

		keyFrame = m_pOutputFrame->key_frame;
		dts = (uint64_t)m_pOutputFrame->pkt_dts;
		pts = (uint64_t)m_pOutputFrame->pts;
		width = m_pOutputFrame->width;
		height = m_pOutputFrame->height;

		return m_pOutputFrame;
	}

	ret = -1;
	return nullptr;
}


void D3DPlayer::D3DDecoder::ReleaseFrame()
{
	av_frame_unref(m_pOutputFrame);
}


bool D3DPlayer::D3DDecoder::IsInitFailed()
{
	return InitFailed;
}


int D3DPlayer::D3DDecoder::GetVideoWidth()
{
	return m_VideoWidth;
}


int D3DPlayer::D3DDecoder::GetVideoHeight()
{
	return m_VideoHeight;
}


double D3DPlayer::D3DDecoder::GetFramerate()
{
	if (m_VideoFramerate < 1e-15) {
		m_VideoFramerate = (double)m_pDecoderContext->framerate.den / m_pDecoderContext->framerate.num;
	}
	return m_VideoFramerate;
}


AVCodecID D3DPlayer::D3DDecoder::GetCodecID()
{
	return m_CodecID;
}


AVHWDeviceType D3DPlayer::D3DDecoder::GetHwDeviceType()
{
	return m_HwDeviceType;
}


AVPixelFormat D3DPlayer::D3DDecoder::GetHwPixFormat()
{
	return m_HwPixelFormat;
}


enum AVPixelFormat D3DPlayer::D3DDecoder::GetHwSurfaceFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
	auto pThis = (D3DPlayer::D3DDecoder *)ctx->opaque;

	const enum AVPixelFormat *p;
	for (p = pix_fmts; *p != -1; p++) {
		if (*p == pThis->m_HwPixelFormat) {
			TRACEA(LOG_LEVEL_INFO, "get hw surface format(%s) succeed", av_get_pix_fmt_name(*p));
			return *p;
		}
	}

	TRACEA(LOG_LEVEL_ERROR, "get hw surface format(%s) failed", av_get_pix_fmt_name(pThis->m_HwPixelFormat));

	pThis->InitFailed = true;

	return AV_PIX_FMT_NONE;
}


int D3DPlayer::D3DDecoder::GetPacketPayloadSize()
{
	int size = m_VideoWidth * m_VideoHeight;
	if (size == 0) {
		size = 7680 * 4320;
	}

	return size;
}
