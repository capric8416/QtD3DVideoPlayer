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
	// ģ���ļ�IO����
	m_pAvioBuffer = (uint8_t *)av_malloc(32768);
	m_pAvioContext = avio_alloc_context(m_pAvioBuffer, 32768, 0, (void*)(this), &read_packet, nullptr, nullptr);
}


D3DPlayer::D3DFileMemory::~D3DFileMemory()
{
	// ��Ϊavformat_close_input�����avio_close�����Դ˴������ͷ�
	// libavformat/demux.c#378
	//if (m_pIoContext != nullptr)
	//{
	//	avio_close(m_pIoContext);
	//	m_pIoContext = nullptr;
	//}

	// ��Ϊavio_close�����av_freep�����Դ˴������ͷ�
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
	// �������ļ������ڴ�
	// ���ļ�
	errno_t error = _wfopen_s(&fp, path, L"rb");
	BREAK_FAIL(error == 0, L"fopen_s");

	// ��ȡ�ļ���С
	fseek(fp, 0, SEEK_END);
	m_nBufferSize = ftell(fp);

	// ��ȡ�ļ�����
	rewind(fp);
	m_pBuffer = new uint8_t[m_nBufferSize];
	size_t readSize = fread_s(m_pBuffer, m_nBufferSize, sizeof(uint8_t), m_nBufferSize, fp);
	BREAK_FAIL(readSize == m_nBufferSize, L"fread_s");
	BREAK_LEAVE;
	if (fp != nullptr)
	{
		// �ر��ļ�
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
		m_nBufOffset = 0; // ���¿�ʼ
		return 0;
	}

	memcpy_s(buf, buf_size, m_pBuffer + m_nBufOffset, size);

	m_nBufOffset += size;

	return size;
}
