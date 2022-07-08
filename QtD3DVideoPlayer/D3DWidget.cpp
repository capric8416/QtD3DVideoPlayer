#include "D3DWidget.h"
#include "D3DCommand.h"

#include <QDir>
#include <QResizeEvent>



D3DPlayer::D3DWidget::D3DWidget(
	QWidget *parent
)
	: QWidget(parent)
	, m_pPlayerCmd(&D3DPlayer::D3DPlayerCommand::GetInstance())
{
	// ����DirectX��Ⱦ
	setAutoFillBackground(false);
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_MSWindowsUseDirect3D, true);
	setAttribute(Qt::WA_NoSystemBackground, true);

	//// �������¼�
	//setFocusPolicy(Qt::WheelFocus);

	//// ��������ƶ�
	//setMouseTracking(true);
}


D3DPlayer::D3DWidget::~D3DWidget()
{
}


void D3DPlayer::D3DWidget::paintEvent(QPaintEvent *event)
{
}


void D3DPlayer::D3DWidget::resizeEvent(QResizeEvent *event)
{
}


void D3DPlayer::D3DWidget::Resize(HWND hWnd, int width, int height)
{
	m_pPlayerCmd->Resize((hWnd), width, height);
}


float D3DPlayer::D3DWidget::GetSaturation()
{
	// TODO
	return 0.0f;
}


void D3DPlayer::D3DWidget::SetSaturation(float value)
{
	// TODO
}


float D3DPlayer::D3DWidget::GetContrast()
{
	// TODO
	return 0.0f;
}


void D3DPlayer::D3DWidget::SetContrast(float value)
{
	// TODO
}


float D3DPlayer::D3DWidget::GetBrightness()
{
	// TODO
	return 0.0f;
}


void D3DPlayer::D3DWidget::SetBrightness(float value)
{
	// TODO
}


void D3DPlayer::D3DWidget::DeleteYUVData()
{
	// TODO - fake
}


void D3DPlayer::D3DWidget::SendFrame(uint8_t * pic, uint32_t width, uint32_t height)
{
	// TODO - fake
}


void D3DPlayer::D3DWidget::CreateYUVData(uint8_t * pic, uint32_t width, uint32_t height)
{
	// TODO - fake
}


void D3DPlayer::D3DWidget::SetPainterState(int type, bool flag)
{
	// TODO - fake
}


void D3DPlayer::D3DWidget::SaveImg(HWND hWnd, QString path, QString fileName, QString watermarkPath)
{
	path.append("/");
	path.append(fileName);
	path.replace(QString("/"), QString("\\"));

	watermarkPath.replace(QString("/"), QString("\\"));

	m_pPlayerCmd->TakeSnapshot(hWnd, path.toStdWString().c_str(), watermarkPath.toStdWString().c_str());
}


