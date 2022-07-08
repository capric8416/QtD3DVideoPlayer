#pragma once

#include "D3DCommon.h"

#include <functional>



namespace D3DPlayer
{
	class D3D_PLAYER_EXPORT D3DSnapshot {
	public:
		D3DSnapshot(const wchar_t *pstrSnapshot, const wchar_t *pstrWatermark, std::function<void(void *pTexture)> cbTake)
			: m_cbTake(cbTake)
		{
			wcscpy(SnapshotPath, pstrSnapshot);
			wcscpy(WatermarkPath, pstrWatermark);
		}

		~D3DSnapshot()
		{
		}

		void Take(void *pTexture)
		{
			m_cbTake(pTexture);
		}

		wchar_t SnapshotPath[MAX_PATH];
		wchar_t WatermarkPath[MAX_PATH];

	private:
		std::function<void(void *pTexture)> m_cbTake;
	};


	class D3D_PLAYER_EXPORT D3DRender
	{
	public:
		D3DRender(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight, bool KeepAspectRatio);
		virtual ~D3DRender();

		virtual void Initialize() = 0;
		virtual void Initialize(void *pDevice, void *pContext);
		virtual void Deinitialize() = 0;

		virtual void Draw(AVFrame *pFrame, enum AVCodecID CodecID) = 0;
		virtual void Present() = 0;

		virtual void ResizeSwapChain() = 0;

		virtual void Resize(int width, int height);

		// 如果水印图片路径不为空，则叠加到上面
		virtual bool TakeSnapshot(const wchar_t *pstrSnapshot, const wchar_t *pstrWatermark) = 0;

		HWND GetHWND();

		bool IsInitFailed();

	protected:
		double GetVideoRatio();
		double GetViewRatio();

		bool ScaleByRatio(RECT *pRect);

		void AttachWatermark(const wchar_t *pstrSnapshotPath, const wchar_t *pstrWatermarkPath);
		bool GetEncoderClsid(const WCHAR *pstrFormat, CLSID *pClsid);


		HWND m_hWnd;

		int m_VideoWidth;
		int m_VideoHeight;

		int m_ViewWidth;
		int m_ViewHeight;
		int m_OldViewWidth;
		int m_OldViewHeight;

		bool m_NeedResize;

		bool m_KeepAspectRatio;

		D3DSnapshot *m_pSnapshot;

		bool m_InitFailed;
	};
}
