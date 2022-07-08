#include "D3DImage.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
};



D3DPlayer::D3DJpegImageFromNV12::D3DJpegImageFromNV12()
{
}



D3DPlayer::D3DJpegImageFromNV12::~D3DJpegImageFromNV12()
{
}



bool D3DPlayer::D3DJpegImageFromNV12::Encode(AVFrame *pHwFrame, const wchar_t *pstrImagePath)
{
	const char *path = ConvertToChars(pstrImagePath);

	uint8_t *rgb24 = nullptr;

	BREAK_ENTER;

	AVFrame *pSwFrame = av_frame_alloc();
	BREAK_FAIL(pSwFrame != NULL, L"av_frame_alloc");

	BREAK_FAIL(av_hwframe_transfer_data(pHwFrame, pSwFrame, 0) >= 0, L"av_hwframe_transfer_data");

	int width = pHwFrame->width, height = pHwFrame->height;
	uint8_t *rgb24 = new uint8_t[width * height * 3];
	BREAK_FAIL(rgb24 != NULL, "new uint8_t *");

	Nv12ToRgb24(pSwFrame->data[0], pSwFrame->data[1], rgb24, width, height);

	BYTE buffer[1024];
	memset(buffer, 0, 1024);
	LPBITMAPINFOHEADER  lpBmpInfoHead = (LPBITMAPINFOHEADER)buffer;
	lpBmpInfoHead->biSize = sizeof(BITMAPINFOHEADER);
	lpBmpInfoHead->biBitCount = bit_data.Stride / bit_data.Width * 8;
	lpBmpInfoHead->biWidth = bit_data.Width;
	lpBmpInfoHead->biHeight = bit_data.Height;
	lpBmpInfoHead->biPlanes = 1;
	lpBmpInfoHead->biCompression = BI_RGB;

	Bitmap* bm_mem = Bitmap::FromBITMAPINFO((LPBITMAPINFO)lpBmpInfoHead, data);

	BREAK_LEAVE;

	delete[] path;

	delete[] rgb24;

	if (!succeed) {
		return false;
	}
}


void D3DPlayer::D3DJpegImageFromNV12::Nv12ToRgb24(uint8_t *nv12_y, uint8_t *nv12_uv, uint8_t *rgb24, int width, int height)
{
	int index = 0;
	for (int ih = 0; ih < height; ih++) {
		for (int iw = 0; iw < width; iw++) {
			//YYYYYYYYUVUV
			uint8_t Y = nv12_y[iw + ih * width];
			uint8_t U = nv12_uv[ih / 2 * width + (iw / 2) * 2];
			uint8_t V = nv12_uv[ih / 2 * width + (iw / 2) * 2 + 1];

			rgb24[index++] = Y + 1.402 * (V - 128);  // R
			rgb24[index++] = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128);  // G
			rgb24[index++] = Y + 1.772 * (U - 128);  // B
		}
	}
}
