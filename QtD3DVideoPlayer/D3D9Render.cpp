#include "D3D9Render.h"


extern "C"
{
#include <libavcodec/avcodec.h>
}

#if defined(_DEBUG)
#define D3D_DEBUG_INFO
#endif // _DEBUG
#include <d3d9.h>


#pragma comment(lib, "d3d9.lib")



D3DPlayer::D3D9Render::D3D9Render(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight, bool KeepAspectRatio)
	: D3DRender(hWnd, VideoWidth, VideoHeight, ViewWidth, ViewHeight, KeepAspectRatio)

	, m_pD3D(nullptr)
	, m_pD3DSwapChain(nullptr)
	, m_pD3DDevice(nullptr)
	, m_pD3DBackSurface(nullptr)

	, m_pDestRect(nullptr)
{
	Initialize();
}


D3DPlayer::D3D9Render::~D3D9Render()
{
	Deinitialize();
}



void D3DPlayer::D3D9Render::Initialize()
{
	if (m_pD3D) {
		TRACEA(LOG_LEVEL_WARNING, "Skip initialization because d3d9 was already created");
		return;
	}

	if (!(m_VideoWidth > 0 && m_VideoHeight > 0 && m_ViewWidth > 0 && m_ViewHeight > 0)) {
		TRACEA(LOG_LEVEL_WARNING, "Skip initialization because of invalid parameters: video width = %d, video heigth = %d, view width = %d, view height = %d", m_VideoWidth, m_VideoHeight, m_ViewWidth, m_ViewHeight);
		return;
	}

	TRACEA(LOG_LEVEL_INFO, "<begin>");

	std::lock_guard<std::mutex> locker(m_mutexInitUninit);

	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	TRACEA(LOG_LEVEL_INFO, "</end>");
}


void D3DPlayer::D3D9Render::Deinitialize()
{
	TRACEA(LOG_LEVEL_INFO, "<begin>");

	std::lock_guard<std::mutex> locker(m_mutexInitUninit);

	SafeRelease(m_pD3DBackSurface);

	SafeRelease(m_pD3DDevice);

	SafeRelease(m_pD3DSwapChain);

	SafeRelease(m_pD3D);

	SafeDelete(m_pDestRect);

	TRACEA(LOG_LEVEL_INFO, "</end>");
}


void D3DPlayer::D3D9Render::Draw(AVFrame * pFrame, AVCodecID CodecID)
{
	if (m_VideoWidth == 0 && m_VideoHeight == 0) {
		m_VideoWidth = pFrame->width;
		m_VideoHeight = pFrame->height;

		Initialize();
	}
	else {
		m_VideoWidth = pFrame->width;
		m_VideoHeight = pFrame->height;
	}

	bool resized = false;
	if (m_NeedResize)
	{
		ResizeSwapChain();

		resized = true;
		m_NeedResize = false;

		UpdateDestRectByRatio();
	}

	IDirect3DSurface9 *pSurface = (IDirect3DSurface9 *)pFrame->data[3];

	bool setted = GetDevice(pSurface);

	CreateAdditionalSwapChain();

	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_pD3DBackSurface), L"GetBackBuffer");
	BREAK_ON_FAIL(m_pD3DBackSurface == nullptr ? -1 : 0, L"GetBackBuffer");

	if (setted || resized) {
		if (setted) {
			UpdateDestRectByRatio();
		}
		result = m_pD3DDevice->ColorFill((IDirect3DSurface9 *)m_pD3DBackSurface, NULL, D3DCOLOR_RGBA(128, 128, 128, 1));
		if (FAILED(result)) {
			TRACEA(LOG_LEVEL_WARNING, "Clear render target error with code: 0x%0x", result);
		}
	}

	result = IDirect3DDevice9_SetSamplerState(m_pD3DDevice, 0, D3DSAMP_MINFILTER, D3DTEXF_GAUSSIANQUAD);
	if (FAILED(result)) {
		TRACEA(LOG_LEVEL_WARNING, "IDirect3DDevice9_SetSamplerState D3DSAMP_MINFILTER D3DTEXF_GAUSSIANQUAD error with code: 0x%0x", result);
	}
	result = IDirect3DDevice9_SetSamplerState(m_pD3DDevice, 0, D3DSAMP_MAGFILTER, D3DTEXF_GAUSSIANQUAD);
	if (FAILED(result)) {
		TRACEA(LOG_LEVEL_WARNING, "IDirect3DDevice9_SetSamplerState D3DSAMP_MAGFILTER D3DTEXF_GAUSSIANQUAD error with code: 0x%0x", result);
	}
	result = IDirect3DDevice9_SetSamplerState(m_pD3DDevice, 0, D3DSAMP_MIPFILTER, D3DTEXF_GAUSSIANQUAD);
	if (FAILED(result)) {
		TRACEA(LOG_LEVEL_WARNING, "IDirect3DDevice9_SetSamplerState D3DSAMP_MIPFILTER D3DTEXF_GAUSSIANQUAD error with code: 0x%0x", result);
	}

	// ffmpeg/libavcodecdxva2.c#613
	int surfaceAlignment;
	if (CodecID == AV_CODEC_ID_HEVC || CodecID == AV_CODEC_ID_AV1) {
		surfaceAlignment = 128;
	}
	else {
		surfaceAlignment = 16;
	}
	int widthAligned = FFALIGN(m_VideoWidth, surfaceAlignment);
	int heightAligned = FFALIGN(m_VideoHeight, surfaceAlignment);
	if (widthAligned == m_VideoWidth && heightAligned == m_VideoHeight) {
		BREAK_ON_FAIL(m_pD3DDevice->StretchRect(pSurface, NULL, m_pD3DBackSurface, m_pDestRect, D3DTEXF_NONE), L"StretchRect");
	}
	else {
		RECT src;
		src.left = 0, src.top = 0, src.right = m_VideoWidth, src.bottom = m_VideoHeight;
		BREAK_ON_FAIL(m_pD3DDevice->StretchRect(pSurface, &src, m_pD3DBackSurface, m_pDestRect, D3DTEXF_NONE), L"StretchRect");
	}

	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D9Render::Present()
{
	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DSwapChain->Present(NULL, NULL, NULL, NULL, NULL), L"Present");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D9Render::ResizeSwapChain()
{
	if (m_pD3DSwapChain == nullptr)
	{
		return;
	}

	SafeRelease(m_pD3DSwapChain);

	CreateAdditionalSwapChain();
}



bool D3DPlayer::D3D9Render::GetDevice(IDirect3DSurface9 *pSurface)
{
	if (m_pD3DDevice == nullptr) {
		BREAK_ON_FAIL_ENTER;
		BREAK_ON_FAIL(pSurface->GetDevice(&m_pD3DDevice), L"GetDevice");
		BREAK_ON_FAIL_LEAVE;
		//BREAK_ON_FAIL_CLEAN;

		m_InitFailed = FAILED(result);

		return true;
	}

	return false;
}


void D3DPlayer::D3D9Render::CreateAdditionalSwapChain()
{
	if (m_pD3DSwapChain == nullptr) {
		D3DPRESENT_PARAMETERS params = {};
		params.Windowed = TRUE;
		params.hDeviceWindow = m_hWnd;
		params.BackBufferFormat = D3DFORMAT::D3DFMT_X8R8G8B8;
		params.BackBufferWidth = m_ViewWidth;
		params.BackBufferHeight = m_ViewHeight;
		params.SwapEffect = D3DSWAPEFFECT_DISCARD;
		params.BackBufferCount = 1;
		params.Flags = 0;
		BREAK_ON_FAIL_ENTER;
		BREAK_ON_FAIL(m_pD3DDevice->CreateAdditionalSwapChain(&params, &m_pD3DSwapChain), L"CreateAdditionalSwapChain");
		BREAK_ON_FAIL(m_pD3DSwapChain == nullptr ? -1 : 0, L"CreateAdditionalSwapChain");
		BREAK_ON_FAIL_LEAVE;
		//BREAK_ON_FAIL_CLEAN;

		m_InitFailed = FAILED(result);
	}
}


void D3DPlayer::D3D9Render::UpdateDestRectByRatio()
{
	if (!m_KeepAspectRatio) {
		return;
	}

	RECT rect;
	if (!ScaleByRatio(&rect)) {
		return;
	}

	if (m_pDestRect == nullptr)
	{
		m_pDestRect = new RECT;
	}

	m_pDestRect->left = rect.left;
	m_pDestRect->top = rect.top;
	m_pDestRect->right = rect.right;
	m_pDestRect->bottom = rect.bottom;
}
