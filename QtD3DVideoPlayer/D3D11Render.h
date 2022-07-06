#pragma once


#include "D3DRender.h"

#include <mutex>



namespace D3DPlayer
{
	class D3D_PLAYER_EXPORT D3D11Render : public D3DRender
	{
	public:
		D3D11Render(HWND hWnd, int VideoWidth, int VideoHeight, int ViewWidth, int ViewHeight, bool KeepAspectRatio = false);
		~D3D11Render();

		void Initialize();
		void Deinitialize();

		virtual void Draw(AVFrame *pFrame, enum AVCodecID CodecID);
		virtual void Present();

		virtual void ResizeSwapChain();


	private:
		void SetViewPoint();
		void CreateSwapChain();
		void CreateSharedTexture();
		void CreateShaderResourceView();
		void CreateRenderTargetView();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateShaders();
		void CreateSamplerState();
		void UpdateVertexBufferByRatio();


		IDXGISwapChain *m_pDXGISwapChain;
		ID3D11Device *m_pD3DDevice;
		ID3D11DeviceContext *m_pD3DDeviceContext;
		ID3D11Texture2D *m_pBackTexture;
		HANDLE m_pSharedHandle;
		ID3D11Texture2D *m_pSharedTexture;
		ID3D11ShaderResourceView *m_pLuminanceView;
		ID3D11ShaderResourceView *m_pChrominanceView;
		ID3D11RenderTargetView * m_pD3DRenderTargetView;
		ID3D11Buffer *m_pVertexBuffer;
		ID3D11Buffer *m_pIndexBuffer;
		ID3D11InputLayout *m_pInputLayout;
		ID3D11VertexShader *m_pVertexShader;
		ID3D11PixelShader *m_pPixelShader;
		ID3D11SamplerState *m_pSamplerLinear;

		size_t m_IndicesSize;

		std::mutex m_mutexInitUninit;
	};
}