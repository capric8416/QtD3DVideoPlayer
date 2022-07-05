#pragma once

#include <QWidget>

#include "D3DCommand.h"



namespace D3DPlayer
{
	class D3DPLAYER_EXPORT D3DWidget : public QWidget
	{
	public:
		explicit D3DWidget(QWidget *parent = nullptr);
		virtual ~D3DWidget();
		QPaintEngine *paintEngine() const { return nullptr; };


	public:
		virtual void paintEvent(QPaintEvent *event);
		virtual void resizeEvent(QResizeEvent *event);

		void Resize(HWND hWnd, int width, int height);

		// TODO
		float GetSaturation();
		void SetSaturation(float value);
		float GetContrast();
		void SetContrast(float value);
		float GetBrightness();
		void SetBrightness(float value);

		// TODO - fake
		void DeleteYUVData();
		void SendFrame(uint8_t *pic, uint32_t width, uint32_t height);
		void CreateYUVData(uint8_t *pic, uint32_t width, uint32_t height);
		void SetPainterState(int type, bool flag);
		void SaveImg(QString path, QString fileName, QString watermarkPath);


	private:
		D3DPlayer::D3DPlayerCommand *m_pPlayerCmd;
	};
}
