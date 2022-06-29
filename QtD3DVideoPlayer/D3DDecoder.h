#pragma once

#include "D3DCommon.h"



namespace D3DPlayer
{
	class D3DDecoder
	{
	public:
		D3DDecoder(bool UseD3D9);
		~D3DDecoder();

		void Initialize();
		void Deinitialization();

		AVFrame *AcquireFrame();
		void ReleaseFrame();

		int GetWidth();
		int GetHeight();

		double GetFramerate();

		enum AVHWDeviceType GetHWDeviceType();

	private:
		D3DPlayer::D3DFileMemory *m_pFileMemory;

		AVCodec *m_pDecoder;
		AVCodecContext *m_pDecoderContex;
		AVFormatContext *m_pFormatContex;
		AVBufferRef *m_pHWDeviceCtx;
		AVPacket *m_pInputPacket;
		AVFrame *m_pOutputFrame;

		enum AVHWDeviceType m_HWDeviceType;

		int m_Videoindex;

		int m_VideoWidth;
		int m_VideoHeight;

		double m_VideoFramerate;
	};
}
