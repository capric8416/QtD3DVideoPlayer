#pragma once

#include "D3DCommon.h"
#include <functional>
#include <mutex>



namespace D3DPlayer
{
	class D3D_PLAYER_EXPORT D3DDecoder
	{
	public:
		D3DDecoder(bool UseD3D9);
		~D3DDecoder();

		void Initialize(enum AVCodecID CodecID);
		void Deinitialize();

		int SendPacket(uint8_t *pBuffer, int size, int64_t dts, int64_t pts);
		AVFrame *DecodeFrame(int &ret, int &keyFrame, uint64_t &dts, uint64_t &pts, int &width, int &height);
		void ReleaseFrame();

		bool IsInitFailed();

		int GetVideoWidth();
		int GetVideoHeight();

		double GetFramerate();

		enum AVCodecID GetCodecID();
		enum AVHWDeviceType GetHwDeviceType();
		enum AVPixelFormat GetHwPixFormat();

	private:
		static enum AVPixelFormat GetHwSurfaceFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);

		inline int GetPacketPayloadSize();


		AVCodec *m_pDecoder;
		AVCodecContext *m_pDecoderContext;
		AVBufferRef *m_pHwDeviceContext;
		AVFrame *m_pOutputFrame;

		enum AVCodecID m_CodecID;
		enum AVHWDeviceType m_HwDeviceType;
		enum AVPixelFormat m_HwPixelFormat;

		int m_Videoindex;

		int m_VideoWidth;
		int m_VideoHeight;

		double m_VideoFramerate;

		bool InitFailed;

		std::mutex m_mutexInitUninit;
	};
}
