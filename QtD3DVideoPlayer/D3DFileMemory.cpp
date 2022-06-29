#include "D3DFileMemory.h"

#include "D3DCommon.h"


extern "C"
{
#include <libavformat/avformat.h>
}



uint8_t *D3DPlayer::D3DFileMemory::m_pBuffer(nullptr);
uint64_t D3DPlayer::D3DFileMemory::m_nBufferSize(0);


int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	return static_cast<D3DPlayer::D3DFileMemory*>(opaque)->Read(buf, buf_size);
}


D3DPlayer::D3DFileMemory::D3DFileMemory()
	: m_pAvioContext(nullptr)

	, m_pAvioBuffer(nullptr)

	, m_nBufOffset(0)
{
	// 模拟文件IO操作
	m_pAvioBuffer = (uint8_t *)av_malloc(32768);
	m_pAvioContext = avio_alloc_context(m_pAvioBuffer, 32768, 0, (void*)(this), &read_packet, nullptr, nullptr);
}


D3DPlayer::D3DFileMemory::~D3DFileMemory()
{
	// 因为avformat_close_input会调用avio_close，所以此处无需释放
	// libavformat/demux.c#378
	//if (m_pIoContext != nullptr)
	//{
	//	avio_close(m_pIoContext);
	//	m_pIoContext = nullptr;
	//}

	// 因为avio_close会调用av_freep，所以此处无需释放
	// libavformat/aviobuf.c#1199
	//if (m_pIoBuf != nullptr)
	//{
	//	av_free(m_pIoBuf);
	//	m_pIoBuf = nullptr;
	//}
}


void D3DPlayer::D3DFileMemory::Load(const wchar_t *path)
{
	FILE *fp = nullptr;

	BREAK_ENTER;
	// 将整个文件读进内存
	// 打开文件
	errno_t error = _wfopen_s(&fp, path, L"rb");
	BREAK_FAIL(error == 0, L"fopen_s");

	// 读取文件大小
	fseek(fp, 0, SEEK_END);
	m_nBufferSize = ftell(fp);

	// 读取文件内容
	rewind(fp);
	m_pBuffer = new uint8_t[m_nBufferSize];
	size_t readSize = fread_s(m_pBuffer, m_nBufferSize, sizeof(uint8_t), m_nBufferSize, fp);
	BREAK_FAIL(readSize == m_nBufferSize, L"fread_s");
	BREAK_LEAVE;
	if (fp != nullptr)
	{
		// 关闭文件
		fclose(fp);
	}
	BREAK_CLEAN;
}


void D3DPlayer::D3DFileMemory::Unload()
{
	SafeDeleteArray(m_pBuffer);
}


AVIOContext* D3DPlayer::D3DFileMemory::GetAvioContext()
{
	return m_pAvioContext;
}


int D3DPlayer::D3DFileMemory::Read(uint8_t *buf, int buf_size)
{
	int size = buf_size;
	if (size > m_nBufferSize - m_nBufOffset) {
		size = (int)(m_nBufferSize - m_nBufOffset);
	}

	if (size <= 0) {
		m_nBufOffset = 0; // 重新开始
		return 0;
	}

	memcpy_s(buf, buf_size, m_pBuffer + m_nBufOffset, size);

	m_nBufOffset += size;

	return size;
}
