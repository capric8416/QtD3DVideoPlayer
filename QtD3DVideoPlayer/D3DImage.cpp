#include "D3DImage.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
};



D3DPlayer::D3DJpegImageFromNV12::D3DJpegImageFromNV12()
{
	m_pFormatContext = avformat_alloc_context();
	m_pOutputFormat = (AVOutputFormat *)av_guess_format("mjpeg", NULL, NULL);
	m_pFormatContext->oformat = m_pOutputFormat;
}



D3DPlayer::D3DJpegImageFromNV12::~D3DJpegImageFromNV12()
{
}



bool D3DPlayer::D3DJpegImageFromNV12::Encode(AVFrame *pVideoFrame, const wchar_t *pstrImagePath)
{
	const char *path = ConvertToChars(pstrImagePath);

	BREAK_ENTER;

	BREAK_FAIL(avio_open(&m_pFormatContext->pb, path, AVIO_FLAG_WRITE) >= 0, pstrImagePath)

	AVStream *pStream = avformat_new_stream(m_pFormatContext, 0);
	BREAK_FAIL(pStream != NULL, L"avformat_new_stream");

	m_pEncoder = (AVCodec *)avcodec_find_encoder(pStream->codecpar->codec_id);
	BREAK_FAIL(m_pEncoder != NULL, L"avcodec_find_encoder");

	m_pEncoderContext = (AVCodecContext *)avcodec_alloc_context3(m_pEncoder);
	BREAK_FAIL(m_pEncoderContext != NULL, L"avcodec_alloc_context3");

	BREAK_FAIL(avcodec_parameters_to_context(m_pEncoderContext, pStream->codecpar) >= 0, L"avcodec_parameters_to_context");

	BREAK_FAIL(avcodec_open2(m_pEncoderContext, m_pEncoder, NULL) >= 0, L"avcodec_open2");

	AVFrame *m_pPictureFrame = av_frame_alloc();
	int size = av_image_get_buffer_size(m_pEncoderContext->pix_fmt, m_pEncoderContext->width, m_pEncoderContext->height, m_pEncoderContext->block_align);

	uint8_t *pPictureBuffer = (uint8_t *)av_malloc(size);
	BREAK_FAIL(pPictureBuffer != NULL, L"av_malloc");

	av_image_fill_pointers(&m_pPictureFrame, pPictureBuffer, m_pEncoderContext->pix_fmt, m_pEncoderContext->width, m_pEncoderContext->height);

	BREAK_LEAVE;

	delete[] path;

	if (!succeed) {
		return false;
	}
}
