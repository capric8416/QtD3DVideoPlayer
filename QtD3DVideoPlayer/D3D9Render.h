#pragma once


#include "D3DRender.h"


namespace D3DPlayer
{
	class D3D9Render : public D3DRender
	{
	public:
		D3D9Render(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight);
		~D3D9Render();

		void Initialize();
		void Deinitialization();

		virtual void Draw(AVFrame *pFrame);
		virtual void Present();

		virtual void ResizeSwapChain();

	private:
		void GetDevice(IDirect3DSurface9 *pSurface);
		void CreateAdditionalSwapChain();

		void UpdateDestRectByRatio();


		IDirect3D9 *m_pD3D;
		IDirect3DSwapChain9 *m_pD3DSwapChain;
		IDirect3DDevice9 *m_pD3DDevice;
		IDirect3DSurface9 *m_pD3DBackSurface;

		RECT *m_pDestRect;
	};
}