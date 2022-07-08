#pragma once


#include "D3DCommon.h"



namespace D3DPlayer
{
	class D3DImage
	{
	public:
		D3DImage() {};
		virtual ~D3DImage() {};

		virtual bool Encode(AVFrame *pVideoFrame, const wchar_t *pstrImagePath) = 0;
	};


	class D3DJpegImageFromNV12 : public D3DImage {
	public:
		D3DJpegImageFromNV12();
		~D3DJpegImageFromNV12();

		virtual bool Encode(AVFrame *pHwFrame, const wchar_t *pstrImagePath);


	private:
		void Nv12ToRgb24(uint8_t *nv12_y, uint8_t *nv12_uv, uint8_t *rgb24, int width, int height);
	};
}

