#pragma once

#include "D3DCommon.h"



namespace D3DPlayer
{
	class D3DPLAYER_EXPORT D3DRender
	{
	public:
		D3DRender(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight);
		virtual ~D3DRender();

		virtual void Initialize() = 0;
		virtual void Deinitialize() = 0;

		virtual void Draw(AVFrame *pFrame) = 0;
		virtual void Present() = 0;

		virtual void ResizeSwapChain() = 0;

		HWND GetHWND();

		virtual void Resize(int width, int height);

		bool IsInitFailed();

	protected:
		double GetVideoRatio();
		double GetViewRatio();

		bool ScaleByRatio(RECT *pRect);

		HWND m_hWnd;

		int m_VideoWidth;
		int m_VideoHeight;

		int m_ViewWidth;
		int m_ViewHeight;
		int m_OldViewWidth;
		int m_OldViewHeight;

		bool m_NeedResize;

		bool m_InitFailed;
	};
}
