#pragma once


#include "D3DRender.h"

#include <mutex>


namespace D3DPlayer
{
	class D3D_PLAYER_EXPORT D3D9Render : public D3DRender
	{
	public:
		D3D9Render(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight, bool KeepAspectRatio = false);
		~D3D9Render();

		void Initialize();
		void Deinitialize();

		virtual void Draw(AVFrame *pFrame, enum AVCodecID CodecID);
		virtual void Present();

		virtual void ResizeSwapChain();

	private:
		bool GetDevice(IDirect3DSurface9 *pSurface);
		void CreateAdditionalSwapChain();

		void UpdateDestRectByRatio();


		IDirect3D9 *m_pD3D;
		IDirect3DSwapChain9 *m_pD3DSwapChain;
		IDirect3DDevice9 *m_pD3DDevice;
		IDirect3DSurface9 *m_pD3DBackSurface;

		RECT *m_pDestRect;
		
		std::mutex m_mutexInitUninit;
	};
}