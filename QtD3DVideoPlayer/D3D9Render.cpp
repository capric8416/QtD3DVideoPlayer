#include "D3D9Render.h"


extern "C"
{
#include <libavcodec/avcodec.h>
}

#if defined(D3D_PLAYER_DX_DEBUG)
#define D3D_DEBUG_INFO
#endif // D3D_PLAYER_DX_DEBUG
#include <d3d9.h>
#include <d3dx9tex.h>


#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")



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

	if (m_pD3DDevice != nullptr && m_pD3DBackSurface != nullptr) {
		m_pD3DDevice->ColorFill((IDirect3DSurface9 *)m_pD3DBackSurface, NULL, D3DCOLOR_RGBA(128, 128, 128, 1));
	}

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

	if (m_pSnapshot) {
		m_pSnapshot->Take(pSurface);
		m_pSnapshot = nullptr;
	}

	CreateAdditionalSwapChain(&m_pD3DSwapChain, m_VideoWidth, m_VideoHeight);

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

	CreateAdditionalSwapChain(&m_pD3DSwapChain, m_VideoWidth, m_VideoHeight);
}


bool D3DPlayer::D3D9Render::TakeSnapshot(const wchar_t *pstrSnapshot, const wchar_t *pstrWatermark)
{
	if (m_VideoWidth == 0 || m_VideoHeight == 0 || m_pD3DDevice == nullptr) {
		TRACEA(LOG_LEVEL_WARNING, "please wait render to be inited");
		return false;
	}

	if (m_pSnapshot != nullptr) {
		TRACEA(LOG_LEVEL_WARNING, "please wait another snapshot to be finished");
		return false;
	}

	m_pSnapshot = new D3DSnapshot(pstrSnapshot, pstrWatermark, [this](void *pTexture) {
		IDirect3DSwapChain9 *pD3DSwapChain = nullptr;
		IDirect3DSurface9 *pD3DBackSurface = nullptr;

		IDirect3DSurface9 *pSurface = (IDirect3DSurface9 *)pTexture;

		BREAK_ON_FAIL_ENTER;

		CreateAdditionalSwapChain(&pD3DSwapChain, m_VideoWidth, m_VideoHeight);
		BREAK_ON_FAIL(pD3DSwapChain != nullptr ? S_OK : -1, L"CreateAdditionalSwapChain");

		BREAK_ON_FAIL(pD3DSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pD3DBackSurface), L"GetBackBuffer");

		RECT rect;
		rect.left = 0, rect.top = 0, rect.right = m_VideoWidth, rect.bottom = m_VideoHeight;
		BREAK_ON_FAIL(m_pD3DDevice->StretchRect(pSurface, &rect, pD3DBackSurface, &rect, D3DTEXF_NONE), L"StretchRect");

		BREAK_ON_FAIL(D3DXSaveSurfaceToFile(m_pSnapshot->SnapshotPath, D3DXIFF_JPG, pD3DBackSurface, NULL, NULL), L"D3DXSaveSurfaceToFile");
		TRACEW(LOG_LEVEL_INFO, "take rendering snapshot to file %s", m_pSnapshot->SnapshotPath);

		BREAK_ON_FAIL_LEAVE;

		SafeRelease(pD3DBackSurface);

		SafeRelease(pD3DSwapChain);

		// try to paint watermark if needed
		AttachWatermark(m_pSnapshot->SnapshotPath, m_pSnapshot->WatermarkPath);

		SafeDelete(m_pSnapshot);
	});

	return true;
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


void D3DPlayer::D3D9Render::CreateAdditionalSwapChain(IDirect3DSwapChain9 **ppD3DSwapChain, int width, int height)
{
	if (*ppD3DSwapChain == nullptr) {
		D3DPRESENT_PARAMETERS params = {};

		params.BackBufferFormat = D3DFORMAT::D3DFMT_X8R8G8B8;
		params.BackBufferWidth = width;
		params.BackBufferHeight = height;
		params.BackBufferCount = 2;
		params.Flags = 0;
		params.hDeviceWindow = m_hWnd;
		params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		params.SwapEffect = D3DSWAPEFFECT_DISCARD;
		params.Windowed = TRUE;
		BREAK_ON_FAIL_ENTER;
		BREAK_ON_FAIL(m_pD3DDevice->CreateAdditionalSwapChain(&params, ppD3DSwapChain), L"CreateAdditionalSwapChain");
		BREAK_ON_FAIL(*ppD3DSwapChain == nullptr ? -1 : 0, L"CreateAdditionalSwapChain");
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
