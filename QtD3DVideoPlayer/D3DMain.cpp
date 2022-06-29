#include <QtWidgets/QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QScreen>

#include "D3DCommon.h"
#include "D3DFileMemory.h"
#include <VersionHelpers.h>

#include <thread>

#include "D3DWidget.h"


//const bool IS_WINDOWS_7 = false;
const bool IS_WINDOWS_7 = IsWindows7OrGreater() && !IsWindows8OrGreater();




class ResourcesManager
{
public:
	ResourcesManager(QApplication &app)
		: m_DebugMode(false)

		, m_Ways(1)
		, m_Fps(60)

		, m_Count(1)
		, m_Width(1920)
		, m_Height(1080)

		, m_Player(IS_WINDOWS_7)

		, m_ppWidgets(nullptr)
	{
		memset(m_Path, 0, sizeof(wchar_t) * MAX_PATH);

		// 解析命令行参数
		parse(app);

		// 读取264内存
		load();
	}

	~ResourcesManager()
	{
		unload();
	}

	int Run(QApplication &app)
	{
		// 创建播放器
		create();

		// 启动定时器线程
		bool playing = true;
		std::thread timer([&]() {
			LONGLONG interval = 1000 / m_Fps;
			if (IS_WINDOWS_7) {
				interval /= 3;
			}

			while (playing) {
				m_Player.Walk([](D3DPlayer::D3DPlayerCommand *pPlayerCmd, D3DPlayer::D3DPlayerResource *pRes) {
					AVFrame *pFrame = pPlayerCmd->AcquireFrame(pRes);
					if (pFrame) {
						pPlayerCmd->Render(pRes, pFrame);
						pPlayerCmd->ReleaseFrame(pRes);
					}
				});

				SleepMilliseconds(interval);
			}
		});

		int code = app.exec();

		// 等待启动定时器线程结束
		playing = false;
		timer.join();

		// 销毁播放器
		destroy();

		return code;
	}


private:
	// 命令行解析
	void parse(QApplication &app)
	{
		QCoreApplication::setApplicationName("QtVideoPlayer");
		QCoreApplication::setApplicationVersion("1.0");

		QCommandLineParser parser;
		parser.setApplicationDescription("Test FFmpeg decoder and OpenGL render");
		(void)parser.addHelpOption();
		(void)parser.addVersionOption();

		QCommandLineOption debugOption(QStringList() << "d" << "debug",
			QGuiApplication::translate("main", "Enter debug mode (default is false)."),
			QGuiApplication::translate("main", "debug"), "false");
		parser.addOption(debugOption);

		QCommandLineOption waysOption(QStringList() << "w" << "ways",
			QGuiApplication::translate("main", "How many players will be started (default is 4)."),
			QGuiApplication::translate("main", "ways"), "4");
		parser.addOption(waysOption);

		QCommandLineOption fpsOption(QStringList() << "f" << "fps",
			QGuiApplication::translate("main", "Frame rate per second (default is 60)."),
			QGuiApplication::translate("main", "fps"), "60");
		parser.addOption(fpsOption);

		QCommandLineOption pathOption(QStringList() << "p" << "path",
			QGuiApplication::translate("main", "Set video file path"),
			QGuiApplication::translate("main", "path"), "");
		parser.addOption(pathOption);

		parser.process(app);

		// 几路视频
		m_Ways = parser.value(waysOption).toInt();
		if (m_Ways > 36) {
			m_Ways = 36;
		}
		else if (m_Ways < 1) {
			m_Ways = 1;
		}

		// 帧率
		m_Fps = parser.value(fpsOption).toInt();
		if (m_Fps > 60) {
			m_Fps = 60;
		}
		else if (m_Fps < 10) {
			m_Fps = 10;
		}

		// 测试模式
		m_DebugMode = parser.value(debugOption) == "true";
		if (m_DebugMode) {
			m_Ways = 1;
			m_Fps = 30;
			m_Count = 1;
			m_Width = 1920;
			m_Height = 1080;
		}
		else {
			QRect rect = QGuiApplication::primaryScreen()->geometry();

			m_Count = sqrt(m_Ways);
			m_Width = rect.width() / m_Count;
			m_Height = rect.height() / m_Count;
		}

		// 指定视频文件路径
		wcscpy_s(m_Path, parser.value(pathOption).toStdWString().c_str());
	}

	// 将264文件载入内存
	void load()
	{
		if (wcslen(m_Path) == 0) {
			swprintf_s(m_Path, MAX_PATH, L"D:/risley/Media Files/H.264 Videos/Overwatch_Alive_Short_%dx%d.264", m_Width, m_Height);
		}
		TRACE("playing %d video files '%s' at the same time", m_Ways, m_Path);

		D3DPlayer::D3DFileMemory::Load(m_Path);
	}

	// 清理264内存
	void unload()
	{
		D3DPlayer::D3DFileMemory::Unload();
	}

	// 创建播放器窗口
	void create()
	{
		m_ppWidgets = new D3DWidget*[m_Ways];
		for (int i = 0; i < m_Count; i++) {
			for (int j = 0; j < m_Count; j++) {
				D3DWidget *pWidget = new D3DWidget(nullptr, &m_Player, m_Width, m_Height);
				pWidget->setWindowFlag(Qt::FramelessWindowHint);
				pWidget->setGeometry(QRect(i * m_Width, j * m_Height, m_Width, m_Height));
				pWidget->show();
				m_ppWidgets[i * m_Count + j] = pWidget;
			}
		}
	}

	// 销毁播放器窗口
	void destroy()
	{
		// 销毁播放器
		for (int i = 0; i < m_Ways; i++) {
			delete m_ppWidgets[i];
		}
		delete[] m_ppWidgets;
	}

private:
	bool m_DebugMode;  // 测试模式

	int m_Ways;  // 几路播放器
	int m_Fps;  // 帧率

	int m_Count;  // 横向/纵向平铺几路播放器
	int m_Width;  // 单路播放器宽
	int m_Height;  // 单路播放器高

	wchar_t m_Path[MAX_PATH];  // 视频文件路径

	D3DPlayer::D3DPlayerCommand m_Player;  // 视频播放管理器

	D3DWidget **m_ppWidgets;  // 窗口
};



int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	return ResourcesManager(app).Run(app);
}
