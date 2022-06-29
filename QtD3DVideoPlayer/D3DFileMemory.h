#pragma once

#include "D3DCommon.h"



namespace D3DPlayer
{
	// ffmpeg/doc/examples/avio_reading.c
	class D3DFileMemory
	{
	public:
		D3DFileMemory();
		~D3DFileMemory();

		static void Load(const wchar_t *path);
		static void Unload();

		AVIOContext* GetAvioContext();

		int Read(uint8_t *buf, int buf_size);

	private:
		AVIOContext *m_pAvioContext;

		uint8_t *m_pAvioBuffer;

		uint64_t m_nBufOffset;

		static uint8_t *m_pBuffer;
		static uint64_t m_nBufferSize;
	};
}