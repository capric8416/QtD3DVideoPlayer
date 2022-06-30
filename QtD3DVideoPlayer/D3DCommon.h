#pragma once

#include <assert.h>
#include <stdint.h>

#include <Windows.h>
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif // !VC_EXTRALEAN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

// disable build warnings
#pragma warning(disable: 4819)   // warning C4819: 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失


// d3d9 forwards
struct IDirect3D9;
struct IDirect3DSwapChain9;
struct IDirect3DDevice9;
struct IDirect3DSurface9;

// d3d11 forwards
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11SamplerState;

// ffmpeg forwards
enum AVHWDeviceType;
struct AVBufferRef;
struct AVCodec;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVIOContext;
struct AVPacket;
class FileMemory;

// project forwards
namespace D3DPlayer
{
	class D3DRender;
	class D3D9Render;
	class D3D11Render;
	class D3DPlayerCommand;
	class D3DDecoder;
	class D3DFileMemory;
}



template <class T>
inline void SafeDelete(T *&pT)
{
	if (pT != nullptr)
	{
		delete pT;
		pT = nullptr;
	}
}


template <class T>
inline void SafeDeleteArray(T *&pT)
{
	if (pT != nullptr)
	{
		delete[] pT;
		pT = nullptr;
	}
}


template <class T>
inline void SafeRelease(T *&pT)
{
	if (pT != nullptr)
	{
		pT->Release();
		pT = nullptr;
	}
}

template <class T>
inline void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = nullptr;
	}
}


// Trace like printf
inline void Trace(const wchar_t *format, ...);


// 皮秒 < 纳秒 < 微秒 < 毫秒 < 秒
// 100纳秒休眠
int SleepNanoseconds(LONGLONG hundreds);
// 微秒休眠
#define SleepMicrosecond(microseconds)  SleepNanoseconds(10 * microseconds)
// 毫秒休眠
#define SleepMilliseconds(milliseconds) SleepNanoseconds(10000 * milliseconds)


#define LAST_BACKSLASH_POS wcsrchr(__FILEW__, L'\\')
#define FILE_NAME LAST_BACKSLASH_POS ? LAST_BACKSLASH_POS + 1 : __FILEW__
#define TRACE(format, ...)                               \
			Trace(                                       \
				L"[%s %d %s] " ## format ## "\n",        \
				FILE_NAME,                               \
				__LINE__,                                \
				__FUNCTIONW__,                           \
				__VA_ARGS__                              \
			)



#define BREAK_ON_FAIL_ENTER              HRESULT result = 0;                           \
										 do {
#define BREAK_ON_FAIL(hr, error)             result = 0;                               \
											 if (FAILED(hr)) {                         \
												 TRACE("%s, hr = 0x%x", error, hr);    \
												 result = hr;                          \
												 break;                                \
											 }
#define BREAK_ON_FAIL_LEAVE              } while (false)
#ifdef _DEBUG
#define BREAK_ON_FAIL_CLEAN              assert(result == 0)
#else
#define BREAK_ON_FAIL_CLEAN              (void)0
#endif



#define BREAK_ENTER                      bool success = false;                         \
										 do {
#define BREAK_FAIL(result, error)           success = true;                            \
											if (!result) {                             \
												TRACE("%s, hr = 0x%x", error, result); \
												success = false;                       \
							     				break;                                 \
											}
#define BREAK_LEAVE                      } while (false)
#ifdef _DEBUG
#define BREAK_CLEAN                      assert(success)
#else
#define BREAK_CLEAN                      (void)0
#endif

