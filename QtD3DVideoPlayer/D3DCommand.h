#pragma once


#include "D3DCommon.h"
#include "D3DDecoder.h"

#include <map>
#include <mutex>


namespace D3DPlayer
{
	class D3DPlayerResource {
	public:
		D3DPlayerResource(
			UINT64 no,
			HWND hWnd,
			D3DDecoder *pDecoder,
			D3DRender *pRender
		)
			: No(no)
			, Wnd(hWnd)
			, Render(pRender)
			, Decoder(pDecoder)
		{
		}


		UINT64 No;
		HWND Wnd;
		D3DDecoder *Decoder;
		D3DRender *Render;
	};


	typedef void (CALLBACK* WalkProc)(D3DPlayerCommand *pPlayerCmd, D3DPlayerResource *pRes);


	class D3DPlayerCommand
	{
	public:
		D3DPlayerCommand(bool UseD3D9);
		~D3DPlayerCommand();

		// create decoder and render
		D3DPlayerResource *Create(HWND hWnd, int width, int height);
		// destroy decoder and render
		void Destroy(D3DPlayerResource *pRes);

		// proxy: get decoder's video width
		int GetWidth(D3DPlayerResource *pRes);
		// proxy: get decoder's video height
		int GetHeight(D3DPlayerResource *pRes);

		// proxy: read some packet and decode a frame
		AVFrame *AcquireFrame(D3DPlayerResource *pRes);
		// proxy: release a decodeed frame
		void ReleaseFrame(D3DPlayerResource *pRes);

		// proxy: render a frame
		void Render(D3DPlayerResource *pRes, AVFrame *pFrame);
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
