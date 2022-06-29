#pragma once

#include <QWidget>

#include "D3DCommand.h"


class D3DWidget : public QWidget
{
	Q_OBJECT

public:
	explicit D3DWidget(QWidget *parent = nullptr, D3DPlayer::D3DPlayerCommand *pPlayerCmd = nullptr, int width = 0, int height = 0);
	virtual ~D3DWidget();
	QPaintEngine *paintEngine() const { return nullptr; };


public:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);

	void Initialize(int width, int height);
	void Deinitialization();

	void Resize(int width, int height);
	
	//void Update(AVFrame *pFrame);
	//void Render();
	//void Release();


private:
	//AVFrame *m_pFrame;
	D3DPlayer::D3DPlayerCommand *m_pPlayerCmd;
	D3DPlayer::D3DPlayerResource *m_pPlayerRes;
};

