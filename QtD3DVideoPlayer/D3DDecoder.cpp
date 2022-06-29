#include "D3DDecoder.h"

#include "D3DCommon.h"
#include "D3DFileMemory.h"



extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
}


#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")



D3DPlayer::D3DDecoder::D3DDecoder(bool UseD3D9)
	: m_pFileMemory(nullptr)

	, m_pDecoder(nullptr)
	, m_pDecoderContex(nullptr)
	, m_pFormatContex(nullptr)
	, m_pHWDeviceCtx(nullptr)
	, m_pInputPacket(nullptr)
	, m_pOutputFrame(nullptr)

	, m_HWDeviceType(AV_HWDEVICE_TYPE_NONE)

	, m_Videoindex(-1)

	, m_VideoWidth(0)
	, m_VideoHeight(0)

	, m_VideoFramerate(0)
{
	m_HWDeviceType = UseD3D9 ? AVHWDeviceType::AV_HWDEVICE_TYPE_DXVA2 : AVHWDeviceType::AV_HWDEVICE_TYPE_D3D11VA;

	Initialize();
}


D3DPlayer::D3DDecoder::~D3DDecoder()
{
	Deinitialization();
}


void D3DPlayer::D3DDecoder::Initialize()
{
	BREAK_ENTER;
	m_pFormatContex = avformat_alloc_context();

	m_pFileMemory = new D3DPlayer::D3DFileMemory();
	m_pFormatContex->pb = m_pFileMemory->GetAvioContext();

	int ret = 0;
	ret = avformat_open_input(&m_pFormatContex, "", nullptr, nullptr);
	BREAK_FAIL(ret == 0, L"avformat_open_input");
	ret = avformat_find_stream_info(m_pFormatContex, nullptr);
	BREAK_FAIL(ret == 0, L"avformat_find_stream_info");

	AVStream *pVideoStream = nullptr;
	for (unsigned int i = 0; i < m_pFormatContex->nb_streams; i++) {
		if (m_pFormatContex->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && (m_pFormatContex->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264 || m_pFormatContex->streams[i]->codecpar->codec_id == AV_CODEC_ID_H265)) {
			m_Videoindex = (int)i;
			pVideoStream = m_pFormatContex->streams[i];
			break;
		}
	}
	BREAK_FAIL(pVideoStream, L"avformat_find_stream_info");

	m_pDecoder = (AVCodec *)avcodec_find_decoder(pVideoStream->codecpar->codec_id);
	m_pDecoderContex = avcodec_alloc_context3(m_pDecoder);
	avcodec_parameters_to_context(m_pDecoderContex, pVideoStream->codecpar);
	ret = avcodec_open2(m_pDecoderContex, m_pDecoder, NULL);
	BREAK_FAIL(ret == 0, L"avcodec_open2");

	av_hwdevice_ctx_create(&m_pHWDeviceCtx, m_HWDeviceType, NULL, NULL, NULL);
	m_pDecoderContex->hw_device_ctx = av_buffer_ref(m_pHWDeviceCtx);

	m_VideoWidth = m_pDecoderContex->width;
	m_VideoHeight = m_pDecoderContex->height;

	m_pInputPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	m_pOutputFrame = av_frame_alloc();
	BREAK_LEAVE;
	BREAK_CLEAN;
}


void D3DPlayer::D3DDecoder::Deinitialization()
{
	SafeDelete(m_pFileMemory);

	if (m_pFormatContex != nullptr) {
		avformat_close_input(&m_pFormatContex);
		m_pFormatContex = nullptr;
	}

	if (m_pInputPacket != nullptr) {
		av_packet_free(&m_pInputPacket);
		m_pInputPacket = nullptr;
	}

	if (m_pOutputFrame != nullptr) {
		av_frame_free(&m_pOutputFrame);
		m_pOutputFrame = nullptr;
	}

	if (m_pDecoderContex != nullptr) {
		av_buffer_unref(&m_pHWDeviceCtx);

		avcodec_free_context(&m_pDecoderContex);
		m_pDecoderContex = nullptr;
	}
}


AVFrame *D3DPlayer::D3DDecoder::AcquireFrame()
{
	while (av_read_frame(m_pFormatContex, m_pInputPacket) >= 0) {
		if (m_pInputPacket->stream_index == m_Videoindex) {
			int ret = avcodec_send_packet(m_pDecoderContex, m_pInputPacket);
			av_packet_unref(m_pInputPacket);
			if (ret < 0) {
				return nullptr;
			}

			while (ret >= 0) {
				ret = avcodec_receive_frame(m_pDecoderContex, m_pOutputFrame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					av_frame_unref(m_pOutputFrame);
					break;
				}
				else if (ret < 0) {
					av_frame_unref(m_pOutputFrame);
					return nullptr;
				}

				return m_pOutputFrame;
			}
		}
	}

	return false;
}


void D3DPlayer::D3DDecoder::ReleaseFrame()
{
	av_frame_unref(m_pOutputFrame);
}


int D3DPlayer::D3DDecoder::GetWidth()
{
	return m_VideoWidth;
}


int D3DPlayer::D3DDecoder::GetHeight()
{
	return m_VideoHeight;
}


double D3DPlayer::D3DDecoder::GetFramerate()
{
	if (m_VideoFramerate < 1e-15) {
		m_VideoFramerate = (double)m_pDecoderContex->framerate.den / m_pDecoderContex->framerate.num;
	}
	return m_VideoFramerate;
}


AVHWDeviceType D3DPlayer::D3DDecoder::GetHWDeviceType()
{
	return m_HWDeviceType;
}
