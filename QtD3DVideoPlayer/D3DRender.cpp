#include "D3DRender.h"


extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <gdiplus.h>


#pragma comment(lib, "gdiplus.lib")



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

	, m_pSnapshot(nullptr)

	, m_NeedResize(false)
{
}


D3DPlayer::D3DRender::~D3DRender()
{

}


void D3DPlayer::D3DRender::Initialize(void *pDevice, void *pContext)
{
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


HWND D3DPlayer::D3DRender::GetHWND()
{
	return m_hWnd;
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


void D3DPlayer::D3DRender::AttachWatermark(const wchar_t *pstrSnapshotPath, const wchar_t *pstrWatermarkPath)
{
	WIN32_FIND_DATA snapshotData;
	HANDLE snapshotHandle = FindFirstFile(pstrSnapshotPath, &snapshotData);

	WIN32_FIND_DATA watermarkData;
	HANDLE watermarkHandle = FindFirstFile(pstrSnapshotPath, &watermarkData);

	// 叠加水印图片
	if (snapshotHandle != INVALID_HANDLE_VALUE && watermarkHandle != INVALID_HANDLE_VALUE) {
		Gdiplus::Bitmap *pSnapshot = nullptr;
		Gdiplus::Bitmap *pWatermark = nullptr;
		Gdiplus::Graphics *pGraph = nullptr;

		BREAK_ENTER;

		const wchar_t *pstrTmpSnapshotPath = std::wstring(std::wstring(pstrSnapshotPath) + L".tmp").c_str();
		BREAK_FAIL(MoveFile(pstrSnapshotPath, pstrTmpSnapshotPath), L"MoveFile");

		pSnapshot = Gdiplus::Bitmap::FromFile(pstrTmpSnapshotPath);
		BREAK_FAIL(pSnapshot != NULL, L"Gdiplus::Bitmap::FromFile");

		pWatermark = Gdiplus::Bitmap::FromFile(pstrWatermarkPath);
		BREAK_FAIL(pWatermark != NULL, L"Gdiplus::Bitmap::FromFile");

		UINT w1 = pSnapshot->GetWidth(), h1 = pSnapshot->GetHeight();
		UINT w2 = pWatermark->GetWidth(), h2 = pWatermark->GetHeight();

		Gdiplus::Bitmap blend(w1, h1);
		BREAK_FAIL(blend.GetLastStatus() == Gdiplus::Ok, L"Gdiplus::Bitmap");

		pGraph = Gdiplus::Graphics::FromImage(&blend);
		BREAK_FAIL(pGraph != NULL, L"Gdiplus::Bitmap");

		BREAK_FAIL(pGraph->DrawImage(pSnapshot, 0, 0, w1, h1) == Gdiplus::Ok, L"DrawImage");
		BREAK_FAIL(pGraph->DrawImage(pWatermark, 0, 0, min(w1, w2), min(h1, h2)) == Gdiplus::Ok, L"DrawImage");

		CLSID jpegClsid;
		BREAK_FAIL(GetEncoderClsid(L"image/jpeg", &jpegClsid), L"GetEncoderClsid");

		BREAK_FAIL(blend.Save(pstrSnapshotPath, &jpegClsid, NULL) == Gdiplus::Ok, L"Save");
		
		SafeDelete(pSnapshot);
		if (!DeleteFile(pstrTmpSnapshotPath)) {
			TRACEW(LOG_LEVEL_WARNING, "delete file %s failed with error 0x%x", pstrTmpSnapshotPath, GetLastError());
		}

		BREAK_LEAVE;

		SafeDelete(pSnapshot);
		SafeDelete(pWatermark);
		SafeDelete(pGraph);
	}
}


bool D3DPlayer::D3DRender::GetEncoderClsid(const WCHAR *pstrFormat, CLSID *pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo *pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return false;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo *)(malloc(size));
	if (pImageCodecInfo == NULL)
		return false;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, pstrFormat) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return true;  // Success
		}
	}

	free(pImageCodecInfo);

	return false;  // Failure
}