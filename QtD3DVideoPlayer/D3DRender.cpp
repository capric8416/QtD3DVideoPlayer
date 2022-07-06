#include "D3DRender.h"


extern "C"
{
#include <libavcodec/avcodec.h>
}



D3DPlayer::D3DRender::D3DRender(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight, bool KeepAspectRatio)
	: m_hWnd(hWnd)

	, m_VideoWidth(VideoWidth)
	, m_VideoHeight(VideoHeight)

	, m_ViewWidth(ViewWidth)
	, m_ViewHeight(ViewHeight)
	, m_OldViewWidth(0)
	, m_OldViewHeight(0)

	, m_InitFailed(false)

	, m_KeepAspectRatio(KeepAspectRatio)

	, m_NeedResize(false)
{
}


D3DPlayer::D3DRender::~D3DRender()
{

}


HWND D3DPlayer::D3DRender::GetHWND()
{
	return m_hWnd;
}


void D3DPlayer::D3DRender::Resize(int width, int height)
{
	if (!m_NeedResize) {
		m_NeedResize = true;

		m_OldViewWidth = m_ViewWidth;
		m_OldViewHeight = m_ViewHeight;
	}

	m_ViewWidth = width;
	m_ViewHeight = height;
}


bool D3DPlayer::D3DRender::IsInitFailed()
{
	return m_InitFailed;
}


double D3DPlayer::D3DRender::GetVideoRatio()
{
	return (double)m_VideoHeight / m_VideoWidth;
}


double D3DPlayer::D3DRender::GetViewRatio()
{
	return (double)m_ViewHeight / m_ViewWidth;
}


bool D3DPlayer::D3DRender::ScaleByRatio(RECT *pRect)
{
	double viewRatio = GetViewRatio();
	double videoRatio = GetVideoRatio();

	// 如果宽高比几乎一致，则无需缩放
	if (fabs(videoRatio - viewRatio) < 1e-15)
	{
		return false;
	}

	// 保持宽高比缩放
	double left, top, width, height;
	if (viewRatio > videoRatio) {
		width = m_ViewWidth;
		height = width * videoRatio;
	}
	else {
		height = m_ViewHeight;
		width = height / videoRatio;
	}
	left = (m_ViewWidth - width) / 2;
	top = (m_ViewHeight - height) / 2;

	pRect->left = (long)left;
	pRect->top = (long)top;
	pRect->right = (long)(width + left);
	pRect->bottom = (long)(height + top);

	return true;
}
