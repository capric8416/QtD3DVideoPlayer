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

		// ���������в���
		parse(app);

		// ��ȡ264�ڴ�
		load();
	}

	~ResourcesManager()
	{
		unload();
	}

	int Run(QApplication &app)
	{
		// ����������
		create();

		// ������ʱ���߳�
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

		// �ȴ�������ʱ���߳̽���
		playing = false;
		timer.join();

		// ���ٲ�����
		destroy();

		return code;
	}


private:
	// �����н���
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

		// ��·��Ƶ
		m_Ways = parser.value(waysOption).toInt();
		if (m_Ways > 36) {
			m_Ways = 36;
		}
		else if (m_Ways < 1) {
			m_Ways = 1;
		}

		// ֡��
		m_Fps = parser.value(fpsOption).toInt();
		if (m_Fps > 60) {
			m_Fps = 60;
		}
		else if (m_Fps < 10) {
			m_Fps = 10;
		}

		// ����ģʽ
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

		// ָ����Ƶ�ļ�·��
		wcscpy_s(m_Path, parser.value(pathOption).toStdWString().c_str());
	}

	// ��264�ļ������ڴ�
	void load()
	{
		if (wcslen(m_Path) == 0) {
			swprintf_s(m_Path, MAX_PATH, L"D:/risley/Media Files/H.264 Videos/Overwatch_Alive_Short_%dx%d.264", m_Width, m_Height);
		}
		TRACE("playing %d video files '%s' at the same time", m_Ways, m_Path);

		D3DPlayer::D3DFileMemory::Load(m_Path);
	}

	// ����264�ڴ�
	void unload()
	{
		D3DPlayer::D3DFileMemory::Unload();
	}

	// ��������������
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

	// ���ٲ���������
	void destroy()
	{
		// ���ٲ�����
		for (int i = 0; i < m_Ways; i++) {
			delete m_ppWidgets[i];
		}
		delete[] m_ppWidgets;
	}

private:
	bool m_DebugMode;  // ����ģʽ

	int m_Ways;  // ��·������
	int m_Fps;  // ֡��

	int m_Count;  // ����/����ƽ�̼�·������
	int m_Width;  // ��·��������
	int m_Height;  // ��·��������

	wchar_t m_Path[MAX_PATH];  // ��Ƶ�ļ�·��

	D3DPlayer::D3DPlayerCommand m_Player;  // ��Ƶ���Ź�����

	D3DWidget **m_ppWidgets;  // ����
};



int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	return ResourcesManager(app).Run(app);
}
