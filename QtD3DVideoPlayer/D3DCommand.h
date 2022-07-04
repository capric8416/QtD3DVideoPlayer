#pragma once


#include "D3DCommon.h"
#include "D3DDecoder.h"

#include <map>
#include <mutex>


namespace D3DPlayer
{
	class D3DPLAYER_EXPORT D3DPlayerResource {
	public:
		D3DPlayerResource(
			UINT64 no,
			HWND hWnd,
			D3DDecoder *pDecoder,
			D3DRender *pRender
		)
			: No(no)
			, Wnd(hWnd)
			, RenderFrame(pRender)
			, Decoder(pDecoder)
		{
		}


		UINT64 No;
		HWND Wnd;
		D3DDecoder *Decoder;
		D3DRender *RenderFrame;
	};

	typedef std::function<void(D3DPlayerCommand *pPlayerCmd, D3DPlayerResource *pRes)> WalkProc;


	class D3DPLAYER_EXPORT D3DPlayerCommand
	{
	private:
		// ����
		D3DPlayerCommand(bool UseD3D9);
		~D3DPlayerCommand();

		D3DPlayerCommand(const D3DPlayerCommand&);
		D3DPlayerCommand& operator=(const D3DPlayerCommand&);

		static void FFmpegLogger(void *avcl, int level, const char *fmt, va_list args);

	public:
		// ����
		static D3DPlayerCommand & GetInstance();

		D3DPlayerResource *Find(HWND hWnd);

		// create decoder and render
		D3DPlayerResource *Create(HWND hWnd, int width, int height);
		// destroy decoder and render
		void Destroy(D3DPlayerResource *pRes);

		// proxy: init decoder
		void DecoderInitialize(D3DPlayerResource *pRes, enum AVCodecID CodeID);
		// proxy: uninit decoder
		void DecoderDeinitialize(D3DPlayerResource *pRes);
		
		// proxy: get codec id
		enum AVCodecID GetCodecID(D3DPlayerResource *pRes);

		// proxy: send a packet to decoder
		int SendPacket(D3DPlayerResource * pRes, uint8_t *pBuffer, int size, int64_t dts, int64_t pts);
		// proxy: read a frame from decoder
		AVFrame *DecodeFrame(D3DPlayerResource * pRes, int &ret, int &keyFrame, uint64_t &dts, uint64_t &pts, int &width, int &height);

		// proxy: render a frame
		void RenderFrame(D3DPlayerResource *pRes, AVFrame *pFrame);
		// proxy: release a decodeed frame
		void ReleaseFrame(D3DPlayerResource *pRes);

		// proxy: resize window
		void Resize(HWND hWnd, int width, int height);

		// iter video player resources
		void Walk(WalkProc proc);


	private:
		std::map<HWND, D3DPlayerResource*> m_mapResources;

		std::mutex m_mutexOperation;

		UINT64 m_No;

		bool m_UseD3D9;
	};
}
