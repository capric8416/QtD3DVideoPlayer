#include "D3DWidget.h"

#include <QResizeEvent>



D3DWidget::D3DWidget(
	QWidget *parent,
	D3DPlayer::D3DPlayerCommand *pPlayerCmd,
	int width,
	int height
)
	: QWidget(parent)
	//, m_pFrame(nullptr)
	, m_pPlayerRes(nullptr)
	, m_pPlayerCmd(pPlayerCmd)
{
	// 允许DirectX渲染
	QWidget::setAttribute(Qt::WA_PaintOnScreen);

	// 允许按键事件
	setFocusPolicy(Qt::WheelFocus);

	// 允许鼠标移动
	setMouseTracking(true);

	Initialize(width, height);
}


D3DWidget::~D3DWidget()
{
	Deinitialization();
}


void D3DWidget::Initialize(int width, int height)
{
	m_pPlayerRes = m_pPlayerCmd->Create((HWND)winId(), width, height);
	assert(m_pPlayerRes);
}


void D3DWidget::Deinitialization()
{
	m_pPlayerCmd->Destroy(m_pPlayerRes);
	m_pPlayerRes = nullptr;
}


void D3DWidget::paintEvent(QPaintEvent *event)
{
	//Render();
}


void D3DWidget::resizeEvent(QResizeEvent *event)
{
	const QSize size = event->size();
	Resize(size.width(), size.height());
}


void D3DWidget::Resize(int width, int height)
{
	m_pPlayerCmd->Resize((HWND)winId(), width, height);
}


//void D3DWidget::Update(AVFrame * pFrame)
//{
//	m_pFrame = pFrame;
//
//	emit update();
//}
//
//
//void D3DWidget::Render()
//{
//	m_pPlayerCmd->Render(m_pPlayerRes, m_pFrame);
//}
//
//
//void D3DWidget::Release()
//{
//	m_pPlayerCmd->ReleaseFrame(m_pPlayerRes);
//}
