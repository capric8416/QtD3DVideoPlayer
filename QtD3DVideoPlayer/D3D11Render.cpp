#include "D3D11Render.h"


extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <vector>

#if defined(D3D_PLAYER_DX_DEBUG)
#define D3D_DEBUG_INFO
#include <dxgidebug.h>
#include <dxgi1_3.h>
#endif // D3D_PLAYER_DX_DEBUG
#include <d3d11.h>
#include <dxgi1_2.h>

#include "D3DVertexShader.h"
#include "D3DPixelShader.h"


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#if defined(D3D_PLAYER_DX_DEBUG)
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#endif // D3D_PLAYER_DX_DEBUG



struct Vertex {
	float x; float y; float z;
	struct {
		float u;
		float v;
	} tex;
};



D3DPlayer::D3D11Render::D3D11Render(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight, bool KeepAspectRatio)
	: D3DRender(hWnd, VideoWidth, VideoHeight, ViewWidth, ViewHeight, KeepAspectRatio)

	, m_pDXGISwapChain(nullptr)
	, m_pD3DDevice(nullptr)
	, m_pD3DDeviceContext(nullptr)
	, m_pBackTexture(nullptr)
	, m_pSharedHandle(nullptr)
	, m_pSharedTexture(nullptr)
	, m_pLuminanceView(nullptr)
	, m_pChrominanceView(nullptr)
	, m_pD3DRenderTargetView(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pInputLayout(nullptr)
	, m_pVertexShader(nullptr)
	, m_pPixelShader(nullptr)
	, m_pSamplerLinear(nullptr)

	, m_IndicesSize(0)

{
	Initialize();
}


D3DPlayer::D3D11Render::~D3D11Render()
{
	Deinitialize();
}


void D3DPlayer::D3D11Render::Initialize()
{
	if (m_pDXGISwapChain) {
		TRACEA(LOG_LEVEL_WARNING, "Skip initialization because swapchain was already created");
		return;
	}

	if (!(m_VideoWidth > 0 && m_VideoHeight > 0 && m_ViewWidth > 0 && m_ViewHeight > 0)) {
		TRACEA(LOG_LEVEL_WARNING, "Skip initialization because of invalid parameters: video width = %d, video heigth = %d, view width = %d, view height = %d", m_VideoWidth, m_VideoHeight, m_ViewWidth, m_ViewHeight);
		return;
	}

	TRACEA(LOG_LEVEL_INFO, "<begin>");

	std::lock_guard<std::mutex> locker(m_mutexInitUninit);

	CreateSwapChain();

	CreateSharedTexture();

	CreateShaderResourceView();

	CreateRenderTargetView();

	CreateVertexBuffer();

	CreateIndexBuffer();

	CreateShaders();

	CreateSamplerState();

	TRACEA(LOG_LEVEL_INFO, "</end>");
}


void D3DPlayer::D3D11Render::Initialize(void *pDevice, void *pContext)
{
	m_pD3DDevice = (ID3D11Device *)pDevice;
	m_pD3DDeviceContext = (ID3D11DeviceContext *)pContext;

	Initialize();
}


void D3DPlayer::D3D11Render::Deinitialize()
{
	TRACEA(LOG_LEVEL_INFO, "<begin>");

	std::lock_guard<std::mutex> locker(m_mutexInitUninit);

	if (m_pD3DDeviceContext != nullptr && m_pD3DRenderTargetView != nullptr) {
		const FLOAT hex808080[] = { 0.5, 0.5, 0.5, 1 };
		m_pD3DDeviceContext->ClearRenderTargetView(m_pD3DRenderTargetView, hex808080);
	}

	SafeRelease(m_pVertexShader);

	SafeRelease(m_pPixelShader);

	SafeRelease(m_pInputLayout);

	SafeRelease(m_pBackTexture);

	SafeRelease(m_pD3DRenderTargetView);

	SafeRelease(m_pSamplerLinear);

	SafeRelease(m_pD3DDeviceContext);

	SafeRelease(m_pD3DDevice);

	SafeRelease(m_pDXGISwapChain);

	SafeRelease(m_pLuminanceView);

	SafeRelease(m_pChrominanceView);

	SafeRelease(m_pSharedTexture);

	if (m_pSharedHandle != nullptr) {
		CloseHandle(m_pSharedHandle);
		m_pSharedHandle = nullptr;
	}

#if defined(D3D_PLAYER_DX_DEBUG)
	IDXGIDebug1 *pDxgiDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDxgiDebug))))
	{
		TRACEA(LOG_LEVEL_INFO, "ReportLiveObjects");
		pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		SafeRelease(&pDxgiDebug);
	}
#endif // D3D_PLAYER_DX_DEBUG

	TRACEA(LOG_LEVEL_INFO, "</end>");
}


void D3DPlayer::D3D11Render::Draw(AVFrame *pFrame, AVCodecID CodecID)
{
	ID3D11Texture2D *pTexture = (ID3D11Texture2D*)pFrame->data[0];
	uint8_t index = (uint8_t)pFrame->data[1];

	BREAK_ON_FAIL_ENTER;

	ID3D11Device *pDevice;
	pTexture->GetDevice(&pDevice);
	BREAK_ON_FAIL(pDevice == nullptr ? -1 : 0, L"GetDevice");

	ID3D11DeviceContext *pDeviceContext;
	pDevice->GetImmediateContext(&pDeviceContext);
	BREAK_ON_FAIL(pDeviceContext == nullptr ? -1 : 0, L"GetImmediateContext");

	if (m_VideoWidth == 0 && m_VideoHeight == 0) {
		m_VideoWidth = pFrame->width;
		m_VideoHeight = pFrame->height;

		Initialize(pDevice, pDeviceContext);
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
	}

	if (m_pSnapshot) {
		m_pSnapshot->Take(m_pSharedTexture);
		m_pSnapshot = nullptr;
	}

	BREAK_ON_FAIL(pDevice->OpenSharedResource(m_pSharedHandle, __uuidof(ID3D11Texture2D), (void**)&m_pSharedTexture), L"OpenSharedResource");
	BREAK_ON_FAIL(m_pSharedTexture == nullptr ? -1 : 0, L"OpenSharedResource");

	pDeviceContext->CopySubresourceRegion(m_pSharedTexture, 0, 0, 0, 0, pTexture, index, 0);
	pDeviceContext->Flush();

	UINT stride = sizeof(Vertex);
	UINT offset = 0u;
	m_pD3DDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pD3DDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pD3DDeviceContext->IASetInputLayout(m_pInputLayout);

	m_pD3DDeviceContext->VSSetShader(m_pVertexShader, 0, 0);

	//if (resized)
	//{
	//	UpdateVertexBufferByRatio();
	//}

	SetViewPoint();

	m_pD3DDeviceContext->PSSetShader(m_pPixelShader, 0, 0);
	m_pD3DDeviceContext->PSSetShaderResources(0, 1, &m_pLuminanceView);
	m_pD3DDeviceContext->PSSetShaderResources(1, 1, &m_pChrominanceView);
	m_pD3DDeviceContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

	m_pD3DDeviceContext->OMSetRenderTargets(1, &m_pD3DRenderTargetView, nullptr);

	const FLOAT hex808080[] = { 0.5, 0.5, 0.5, 1 };
	m_pD3DDeviceContext->ClearRenderTargetView(m_pD3DRenderTargetView, hex808080);

	m_pD3DDeviceContext->DrawIndexed((UINT)m_IndicesSize, 0, 0);

	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::Present()
{
	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pDXGISwapChain->Present(1, 0), L"Present");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::ResizeSwapChain()
{
	if (m_pD3DRenderTargetView == nullptr)
	{
		return;
	}

	SafeRelease(m_pBackTexture);

	SafeRelease(m_pD3DRenderTargetView);

	BREAK_ON_FAIL_ENTER;
	DXGI_SWAP_CHAIN_DESC desc;
	BREAK_ON_FAIL(m_pDXGISwapChain->GetDesc(&desc), L"GetDesc");
	BREAK_ON_FAIL(m_pDXGISwapChain->ResizeBuffers(desc.BufferCount, m_ViewWidth, m_ViewHeight, desc.BufferDesc.Format, desc.Flags), L"ResizeBuffers");

	CreateRenderTargetView();

	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


bool D3DPlayer::D3D11Render::TakeSnapshot(const wchar_t *pstrSnapshot, const wchar_t *pstrWatermark)
{
	if (m_VideoWidth == 0 || m_VideoHeight == 0 || m_pD3DDevice == nullptr) {
		TRACEA(LOG_LEVEL_WARNING, "please wait render to be inited");
		return false;
	}

	if (m_pSnapshot != nullptr) {
		TRACEA(LOG_LEVEL_WARNING, "please wait another snapshot to be finished");
		return false;
	}

	// D3DX11SaveTextureToFile, DirectX::SaveToWICFile 均不支持保持NV12
	m_pSnapshot = new D3DSnapshot(pstrSnapshot, pstrWatermark, [this](void *pTexture) {




		BREAK_ON_FAIL_ENTER;


		BREAK_ON_FAIL_LEAVE;



		// try to paint watermark if needed
		AttachWatermark(m_pSnapshot->SnapshotPath, m_pSnapshot->WatermarkPath);

		SafeDelete(m_pSnapshot);
	});

	return true;
}


void D3DPlayer::D3D11Render::SetViewPoint()
{
	if (m_KeepAspectRatio) {
		RECT rect;
		ScaleByRatio(&rect);

		D3D11_VIEWPORT viewPort = {};
		viewPort.TopLeftX = rect.left;
		viewPort.TopLeftY = rect.top;
		viewPort.Width = (float)(rect.right - rect.left);
		viewPort.Height = (float)(rect.bottom - rect.top);
		viewPort.MaxDepth = 1;
		viewPort.MinDepth = 0;
		m_pD3DDeviceContext->RSSetViewports(1, &viewPort);
	}
	else {

		D3D11_VIEWPORT viewPort = {};
		viewPort.TopLeftX = 0;
		viewPort.TopLeftY = 0;
		viewPort.Width = (float)m_ViewWidth;
		viewPort.Height = (float)m_ViewHeight;
		viewPort.MaxDepth = 1;
		viewPort.MinDepth = 0;
		m_pD3DDeviceContext->RSSetViewports(1, &viewPort);
	}
}


void D3DPlayer::D3D11Render::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	RtlZeroMemory(&desc, sizeof(desc));
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.BufferCount = 2;
	desc.Width = m_ViewWidth;
	desc.Height = m_ViewHeight;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	IDXGIDevice *pDxgiDevice = nullptr;
	IDXGIAdapter *pDxgiAdapter = nullptr;
	IDXGIFactory2 *pDxgiFactory = nullptr;

	BREAK_ON_FAIL_ENTER;

	BREAK_ON_FAIL(m_pD3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDxgiDevice), L"QueryInterface");
	BREAK_ON_FAIL(pDxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDxgiAdapter), L"GetParent");
	BREAK_ON_FAIL(pDxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pDxgiFactory), L"GetParent");
	
	BREAK_ON_FAIL(pDxgiFactory->CreateSwapChainForHwnd(m_pD3DDevice, m_hWnd, &desc, nullptr, nullptr, &m_pDXGISwapChain), L"CreateSwapChainForHwnd");
	BREAK_ON_FAIL(m_pDXGISwapChain == nullptr ? -1 : 0, L"D3D11CreateDeviceAndSwapChain");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	SafeRelease(pDxgiDevice);
	SafeRelease(pDxgiAdapter);
	SafeRelease(pDxgiFactory);

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateSharedTexture()
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Format = DXGI_FORMAT_NV12;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Width = m_VideoWidth;
	desc.Height = m_VideoHeight;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DDevice->CreateTexture2D(&desc, nullptr, &m_pSharedTexture), L"CreateTexture2D");
	BREAK_ON_FAIL(m_pSharedTexture == nullptr ? -1 : 0, L"CreateTexture2D");

	IDXGIResource *pDxgiShare;
	BREAK_ON_FAIL(m_pSharedTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDxgiShare), L"QueryInterface");
	BREAK_ON_FAIL(pDxgiShare->GetSharedHandle(&m_pSharedHandle), L"GetSharedHandle");
	BREAK_ON_FAIL(m_pSharedHandle == nullptr ? -1 : 0, L"GetSharedHandle");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateShaderResourceView()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC const luminancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
		m_pSharedTexture,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		DXGI_FORMAT_R8_UNORM
	);

	D3D11_SHADER_RESOURCE_VIEW_DESC const chrominancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
		m_pSharedTexture,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		DXGI_FORMAT_R8G8_UNORM
	);

	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DDevice->CreateShaderResourceView(m_pSharedTexture, &luminancePlaneDesc, &m_pLuminanceView), L"CreateShaderResourceView");
	BREAK_ON_FAIL(m_pLuminanceView == nullptr ? -1 : 0, L"CreateShaderResourceView");

	BREAK_ON_FAIL(m_pD3DDevice->CreateShaderResourceView(m_pSharedTexture, &chrominancePlaneDesc, &m_pChrominanceView), L"CreateShaderResourceView");
	BREAK_ON_FAIL(m_pChrominanceView == nullptr ? -1 : 0, L"CreateShaderResourceView");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateRenderTargetView()
{
	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_pBackTexture), L"GetBuffer");
	BREAK_ON_FAIL(m_pBackTexture == nullptr ? -1 : 0, L"GetBuffer");

	CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM);
	BREAK_ON_FAIL(m_pD3DDevice->CreateRenderTargetView(m_pBackTexture, &renderTargetViewDesc, &m_pD3DRenderTargetView), L"CreateRenderTargetView");
	BREAK_ON_FAIL(m_pD3DRenderTargetView == nullptr ? -1 : 0, L"CreateRenderTargetView");

	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateVertexBuffer()
{
	const Vertex vertices[] = {
		{-1,    1,    0,    0,    0},
		{1,     1,    0,    1,    0},
		{1,    -1,    0,    1,    1},
		{-1,   -1,    0,    0,    1},
	};

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof(vertices);
	desc.StructureByteStride = sizeof(Vertex);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = vertices;

	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer), L"CreateBuffer");
	BREAK_ON_FAIL(m_pVertexBuffer == nullptr ? -1 : 0, L"CreateBuffer");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateIndexBuffer()
{
	const UINT16 indices[] = { 0, 1, 2, 0, 2, 3 };
	m_IndicesSize = std::size(indices);

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.ByteWidth = sizeof(indices);
	desc.StructureByteStride = sizeof(UINT16);

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = indices;

	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer), L"CreateBuffer");
	BREAK_ON_FAIL(m_pIndexBuffer == nullptr ? -1 : 0, L"CreateBuffer");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateShaders()
{
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DDevice->CreateInputLayout(desc, (UINT)std::size(desc), g_main_VS, sizeof(g_main_VS), &m_pInputLayout), L"CreateInputLayout");
	BREAK_ON_FAIL(m_pInputLayout == nullptr ? -1 : 0, L"CreateInputLayout");

	BREAK_ON_FAIL(m_pD3DDevice->CreateVertexShader(g_main_VS, sizeof(g_main_VS), nullptr, &m_pVertexShader), L"CreateVertexShader");
	BREAK_ON_FAIL(m_pVertexShader == nullptr ? -1 : 0, L"CreateVertexShader");
	BREAK_ON_FAIL(m_pD3DDevice->CreatePixelShader(g_main_PS, sizeof(g_main_PS), nullptr, &m_pPixelShader), L"CreatePixelShader");
	BREAK_ON_FAIL(m_pPixelShader == nullptr ? -1 : 0, L"CreateVertexShader");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::CreateSamplerState()
{
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER::D3D11_FILTER_ANISOTROPIC;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	BREAK_ON_FAIL_ENTER;
	BREAK_ON_FAIL(m_pD3DDevice->CreateSamplerState(&desc, &m_pSamplerLinear), L"CreateSamplerState");
	BREAK_ON_FAIL(m_pSamplerLinear == nullptr ? -1 : 0, L"CreateSamplerState");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}


void D3DPlayer::D3D11Render::UpdateVertexBufferByRatio()
{
	double viewRatio = GetViewRatio();
	double videoRatio = GetVideoRatio();

	// 如果宽高比几乎一致，则无需缩放
	if (fabs(videoRatio - viewRatio) < 1e-15)
	{
		return;
	}

	Vertex vertices[] = {
		{-1,    1,    0,    0,    0},
		{1,     1,    0,    1,    0},
		{1,    -1,    0,    1,    1},
		{-1,   -1,    0,    0,    1},
	};

	// 保持宽高比缩放
	if (videoRatio > viewRatio) {
		float offset = (float)(viewRatio / videoRatio);

		vertices[0].y = offset;
		vertices[1].y = offset;
		vertices[2].y = -offset;
		vertices[3].y = -offset;
	}
	else if (videoRatio < viewRatio) {
		float offset = (float)(videoRatio / viewRatio);

		vertices[0].x = -offset;
		vertices[1].x = offset;
		vertices[2].x = offset;
		vertices[3].x = -offset;
	}

	BREAK_ON_FAIL_ENTER;
	D3D11_MAPPED_SUBRESOURCE mapped;
	BREAK_ON_FAIL(m_pD3DDeviceContext->Map(m_pVertexBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &mapped), L"Map");
	BREAK_ON_FAIL(&mapped == nullptr ? -1 : 0, L"Map");

	memcpy(mapped.pData, vertices, sizeof(vertices));

	m_pD3DDeviceContext->Unmap(m_pVertexBuffer, 0);
	BREAK_ON_FAIL(m_pVertexBuffer == nullptr ? -1 : 0, L"Unmap");
	BREAK_ON_FAIL_LEAVE;
	//BREAK_ON_FAIL_CLEAN;

	m_InitFailed = FAILED(result);
}
