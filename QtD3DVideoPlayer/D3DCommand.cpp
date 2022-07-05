#include "D3DCommand.h"
#include "D3DDecoder.h"
#include "D3D9Render.h"
#include "D3D11Render.h"

extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

#include <VersionHelpers.h>




D3DPlayer::D3DPlayerCommand::D3DPlayerCommand(bool UseD3D9)
	: m_No(0)
	, m_UseD3D9(UseD3D9)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT::COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		TRACEA(LOG_LEVEL_ERROR, "CoInitializeEx failed with 0x%x", hr);
	}

	//av_log_set_level(AV_LOG_TRACE);
	//av_log_set_callback(FFmpegLogger);
}


D3DPlayer::D3DPlayerCommand::~D3DPlayerCommand()
{
	std::lock_guard<std::mutex> locker(m_mutexOperation);

	for (auto iter = m_mapResources.begin(); iter != m_mapResources.end(); iter++) {
		D3DPlayerResource *p = iter->second;

		SafeDelete(p->Decoder);

		SafeDelete(p->Render);

		PostMessage(p->Wnd, WM_QUIT, 0, 0);
	}

	m_mapResources.clear();

	CoUninitialize();
}


D3DPlayer::D3DPlayerCommand::D3DPlayerCommand(const D3DPlayerCommand &)
{
	// TODO: �ڴ˴�����
}


D3DPlayer::D3DPlayerCommand & D3DPlayer::D3DPlayerCommand::operator=(const D3DPlayerCommand &)
{
	// TODO: �ڴ˴����� return ���
	return *this;
}


void D3DPlayer::D3DPlayerCommand::FFmpegLogger(void *avcl, int level, const char *fmt, va_list args)
{
	const char *pstrLevel = LOG_LEVEL_STR_A[level / 8 + 1];
	size_t offset = strlen(pstrLevel) + 14;

	size_t count = _vscprintf(fmt, args) + 1;
	size_t size = offset + count;
	char  *pstrLog = new char[size];

	sprintf_s(pstrLog, size, "* %s * [ffmpeg] ", pstrLevel);

	_vsnprintf_s(pstrLog + offset, size, size, fmt, args);
	
	OutputDebugStringA(pstrLog);
	
	delete[] pstrLog;
}


D3DPlayer::D3DPlayerCommand & D3DPlayer::D3DPlayerCommand::GetInstance()
{
	// ���������ɾ�̬��ʹ�ã����򱻶��ٶ�̬�����ӵ�ʱ������ɶ���ʵ��
	// Effective C++ -- Meyers' Singleton
	// C++0x֮���ʵ�����̰߳�ȫ��, C++0x֮ǰ�������
	static D3DPlayer::D3DPlayerCommand instance(true);
	// D3D11CreateDevice crash at ffmpeg/libavutil/hwcontext_d3d11va.c#576
	//static D3DPlayer::D3DPlayerCommand instance(IsWindows7OrGreater() && !IsWindows8OrGreater());
	return instance;
}


D3DPlayer::D3DPlayerResource * D3DPlayer::D3DPlayerCommand::Create(HWND hWnd, int width, int height)
{
	std::lock_guard<std::mutex> locker(m_mutexOperation);

	auto iter = m_mapResources.find(hWnd);
	if (iter != m_mapResources.end()) {
		return iter->second;
	}

	m_No++;

	D3DDecoder *pDecoder = new D3DDecoder(m_UseD3D9);

	D3DRender *pRender = nullptr;
	if (m_UseD3D9) {
		pRender = new D3D9Render(hWnd, pDecoder->GetWidth(), pDecoder->GetHeight(), width, height);
	}
	else {
		pRender = new D3D11Render(hWnd, pDecoder->GetWidth(), pDecoder->GetHeight(), width, height);
	}

	D3DPlayerResource *pRes = new D3DPlayerResource(m_No, hWnd, pDecoder, pRender);

	m_mapResources[hWnd] = pRes;

	TRACEA(LOG_LEVEL_INFO, "no: %d, hwnd: 0x%x, decoder: 0x%x, render: 0x%x, res: 0x%x", m_No, hWnd, pDecoder, pRender, pRes);

	return pRes;
}


void D3DPlayer::D3DPlayerCommand::Destroy(D3DPlayerResource * pRes)
{
	std::lock_guard<std::mutex> locker(m_mutexOperation);

	auto iter = m_mapResources.find(pRes->Wnd);
	if (iter == m_mapResources.end()) {
		return;
	}

	m_mapResources.erase(iter);
}


void D3DPlayer::D3DPlayerCommand::DecoderInitialize(D3DPlayerResource * pRes, enum AVCodecID CodeID)
{
	if (pRes == nullptr) {
		return;
	}

	pRes->Decoder->Initialize(CodeID);
}


void D3DPlayer::D3DPlayerCommand::DecoderDeinitialize(D3DPlayerResource * pRes)
{
	if (pRes == nullptr) {
		return;
	}

	pRes->Decoder->Deinitialize();
}


bool D3DPlayer::D3DPlayerCommand::DecoderInitializeFailed(D3DPlayerResource * pRes)
{
	if (pRes == nullptr) {
		return false;
	}

	return pRes->Decoder->IsInitFailed();
}


AVCodecID D3DPlayer::D3DPlayerCommand::GetCodecID(D3DPlayer::D3DPlayerResource *pRes)
{
	if (pRes == nullptr) {
		return AV_CODEC_ID_NONE;
	}

	return pRes->Decoder->GetCodecID();
}


int D3DPlayer::D3DPlayerCommand::SendPacket(D3DPlayer::D3DPlayerResource *pRes, uint8_t *pBuffer, int size, int64_t dts, int64_t pts)
{
	if (pRes == nullptr) {
		return 0;
	}

	return pRes->Decoder->SendPacket(pBuffer, size, dts, pts);
}


AVFrame *D3DPlayer::D3DPlayerCommand::DecodeFrame(D3DPlayer::D3DPlayerResource *pRes, int &ret, int &keyFrame, uint64_t &dts, uint64_t &pts, int &width, int &height)
{
	if (pRes == nullptr) {
		ret = -1;
		return nullptr;
	}

	return pRes->Decoder->DecodeFrame(ret, keyFrame, dts, pts, width, height);
}


void D3DPlayer::D3DPlayerCommand::RenderDeinitialize(D3DPlayerResource * pRes)
{
	if (pRes == nullptr) {
		return;
	}

	if (m_UseD3D9) {
		D3D9Render *pRender = (D3D9Render *)pRes->Render;
		pRender->Deinitialize();
	}
	else {
		D3D11Render *pRender = (D3D11Render *)pRes->Render;
		pRender->Deinitialize();
	}
}


void D3DPlayer::D3DPlayerCommand::RenderFrame(D3DPlayerResource * pRes, AVFrame *pFrame)
{
	if (pRes == nullptr) {
		return;
	}

	if (m_UseD3D9) {
		D3D9Render *pRender = (D3D9Render *)pRes->Render;
		pRender->Draw(pFrame);
		pRender->Present();
	}
	else {
		D3D11Render *pRender = (D3D11Render *)pRes->Render;
		pRender->Draw(pFrame);
		pRender->Present();
	}
}


void D3DPlayer::D3DPlayerCommand::ReleaseFrame(D3DPlayerResource * pRes)
{
	if (pRes == nullptr) {
		return;
	}

	pRes->Decoder->ReleaseFrame();
}


bool D3DPlayer::D3DPlayerCommand::RenderInitializeFailed(D3DPlayerResource * pRes)
{
	if (pRes == nullptr) {
		return false;
	}

	if (m_UseD3D9) {
		D3D9Render *pRender = (D3D9Render *)pRes->Render;
		return pRes->Render->IsInitFailed();
	}
	else {
		D3D11Render *pRender = (D3D11Render *)pRes->Render;
		return pRes->Render->IsInitFailed();
	}
}


void D3DPlayer::D3DPlayerCommand::Resize(HWND hWnd, int width, int height)
{
	auto iter = m_mapResources.find(hWnd);
	if (iter == m_mapResources.end()) {
		return;
	}

	iter->second->Render->Resize(width, height);
}


void D3DPlayer::D3DPlayerCommand::Walk(WalkProc proc)
{
	for (auto iter = m_mapResources.begin(); iter != m_mapResources.end(); iter++) {
		proc(this, iter->second);
	}
}
