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


#ifndef BUILD_STATIC
#  define D3D_PLAYER_EXPORT __declspec(dllexport)
# else
#  define D3D_PLAYER_EXPORT __declspec(dllimport)
#endif

// building swtiches
//#define D3D_PLAYER_DX_DEBUG          // debug d3d, dxgi
//#define D3D_PLAYER_FFMPEG_LOG        // enable ffmpeg log
//#define D3D_PLAYER_FFMPEG_D3D11VA    // use d3d11va hw accel instead of dxva2


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
enum AVCodecID;
enum AVHWDeviceType;
enum AVPixelFormat;
struct AVBufferRef;
struct AVCodec;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVIOContext;
struct AVPacket;

// project forwards
namespace D3DPlayer
{
	class D3DRender;
	class D3D9Render;
	class D3D11Render;
	class D3DPlayerCommand;
	class D3DDecoder;
	class D3DPlayerResource;
	class D3DWidget;
}



template <class T>
inline void D3D_PLAYER_EXPORT SafeDelete(T *&pT)
{
	if (pT != nullptr)
	{
		delete pT;
		pT = nullptr;
	}
}


template <class T>
inline void D3D_PLAYER_EXPORT SafeDeleteArray(T *&pT)
{
	if (pT != nullptr)
	{
		delete[] pT;
		pT = nullptr;
	}
}


template <class T>
inline void D3D_PLAYER_EXPORT SafeRelease(T *&pT)
{
	if (pT != nullptr)
	{
		pT->Release();
		pT = nullptr;
	}
}

template <class T>
inline void D3D_PLAYER_EXPORT SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = nullptr;
	}
}


// Trace like printf
inline void D3D_PLAYER_EXPORT TraceA(const char *format, ...);
inline void D3D_PLAYER_EXPORT TraceW(const wchar_t *format, ...);


// 皮秒 < 纳秒 < 微秒 < 毫秒 < 秒
// 100纳秒休眠
int D3D_PLAYER_EXPORT SleepNanoseconds(LONGLONG hundreds);
// 微秒休眠
#define SleepMicrosecond(microseconds)  SleepNanoseconds(10 * microseconds)
// 毫秒休眠
#define SleepMilliseconds(milliseconds) SleepNanoseconds(10000 * milliseconds)




#define LOG_LEVEL_QUIET    -8
#define LOG_LEVEL_PANIC     0
#define LOG_LEVEL_FATAL     8
#define LOG_LEVEL_ERROR    16
#define LOG_LEVEL_WARNING  24
#define LOG_LEVEL_INFO     32
#define LOG_LEVEL_VERBOSE  40
#define LOG_LEVEL_DEBUG    48
#define LOG_LEVEL_TRACE    56
static const char *LOG_LEVEL_STR_A[] = { "QUIET", "PANIC", "FATAL", "ERROR", "WARNING" , "INFO" , "VERBOSE" , "DEBUG" , "TRACE" };
static const wchar_t *LOG_LEVEL_STR_W[] = { L"QUIET", L"PANIC", L"FATAL", L"ERROR", L"WARNING" , L"INFO" , L"VERBOSE" , L"DEBUG" , L"TRACE" };

#define LAST_BACKSLASH_POS_A strrchr(__FILE__, '\\')
#define LAST_BACKSLASH_POS_W wcsrchr(__FILEW__, L'\\')

#define FILE_NAME_A LAST_BACKSLASH_POS_A ? LAST_BACKSLASH_POS_A + 1 : __FILE__
#define FILE_NAME_W LAST_BACKSLASH_POS_W ? LAST_BACKSLASH_POS_W + 1 : __FILEW__

#define TRACEA(level, format, ...)                       \
			TraceA(                                      \
				"* %s * [%s %d %s] " ## format ## "\n",  \
                LOG_LEVEL_STR_A[level / 8 + 1],          \
				FILE_NAME_A,                             \
				__LINE__,                                \
				__FUNCTION__,                            \
				__VA_ARGS__                              \
			)
#define TRACEW(level, format, ...)                       \
			TraceW(                                      \
				L"* %s * [%s %d %s] " ## format ## "\n", \
                LOG_LEVEL_STR_W[level / 8 + 1],          \
				FILE_NAME_W,                             \
				__LINE__,                                \
				__FUNCTIONW__,                           \
				__VA_ARGS__                              \
			)



#define BREAK_ON_FAIL_ENTER              HRESULT result = 0;                                                \
										 do {
#define BREAK_ON_FAIL(hr, error)             result = 0;                                                    \
											 if (FAILED(hr)) {                                              \
												 TRACEW(LOG_LEVEL_ERROR, "%s, hr = 0x%x", error, hr);       \
												 result = hr;                                               \
												 break;                                                     \
											 }
#define BREAK_ON_FAIL_LEAVE              } while (false)
#define BREAK_ON_FAIL_CLEAN              assert(result == 0)



#define BREAK_ENTER                      bool succeed = false;                                              \
										 do {
#define BREAK_FAIL(result, error)           succeed = true;                                                 \
											if (!result) {                                                  \
												TRACEW(LOG_LEVEL_ERROR, "%s", error);                       \
												succeed = false;                                            \
							     				break;                                                      \
											}
#define BREAK_LEAVE                      } while (false)
#define BREAK_CLEAN                      assert(succeed)
