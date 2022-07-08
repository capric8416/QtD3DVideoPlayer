#pragma once


#include "D3DCommon.h"



namespace D3DPlayer
{
	class D3DImage
	{
	public:
		D3DImage() {};
		virtual ~D3DImage() {};

		virtual bool Encode(AVFrame *pVideoFrame, const wchar_t *pstrImagePath) = 0;
	};


	class D3DJpegImageFromNV12 : public D3DImage {
	public:
		D3DJpegImageFromNV12();
		~D3DJpegImageFromNV12();

		virtual bool Encode(AVFrame *pVideoFrame, const wchar_t *pstrImagePath);


	private:
		AVCodec *m_pEncoder;
		AVCodecContext *m_pEncoderContext;
		AVOutputFormat *m_pOutputFormat;
		AVFormatContext *m_pFormatContext;
	};
}

