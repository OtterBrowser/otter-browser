#include "Application.h"
#include "ActionsManager.h"
#include "BookmarksManager.h"
#include "HistoryManager.h"
#include "SearchesManager.h"
#include "SettingsManager.h"
#include "TransfersManager.h"
#include "WebBackendsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QBuffer>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTranslator>
#include <QtNetwork/QLocalSocket>

namespace Otter
{

Application::Application(int &argc, char **argv) : QApplication(argc, argv),
	m_localServer(NULL)
{
	setApplicationName("Otter");
	setApplicationVersion("0.1.01");

	QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/otter";
	QCommandLineParser *parser = getParser();
	parser->process(arguments());

	if (!parser->value("path").isEmpty())
	{
		path = parser->value("path");

		if (QFileInfo(path).isRelative())
		{
			path = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/otter/profiles/").absoluteFilePath(path);
		}
	}

	path = QFileInfo(path).absoluteFilePath();

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(path.toUtf8());

	const QString identifier = hash.result().toBase64();

	delete parser;

	const QString server = applicationName() + (identifier.isEmpty() ? QString() : QString("-%1").arg(identifier));
	QLocalSocket socket;
	socket.connectToServer(server);

	if (socket.waitForConnected(500))
	{
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		QDataStream out(&buffer);
		out << arguments();

		QTextStream stream(&socket);
		stream << byteArray.toBase64();
		stream.flush();

		socket.waitForBytesWritten();

		return;
	}

	m_localServer = new QLocalServer(this);

	connect(m_localServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

	if (!m_localServer->listen(server))
	{
		if (m_localServer->serverError() == QAbstractSocket::AddressInUseError && QFile::exists(m_localServer->serverName()))
		{
			QFile::remove(m_localServer->serverName());

			m_localServer->listen(server);
		}
	}

	if (!QFile::exists(path))
	{
		QDir().mkpath(path);
	}

	SettingsManager::createInstance(path + "/otter.conf", this);

	QSettings defaults(":/schemas/options.ini", QSettings::IniFormat, this);
	const QStringList groups = defaults.childGroups();

	for (int i = 0; i < groups.count(); ++i)
	{
		defaults.beginGroup(groups.at(i));

		const QStringList keys = defaults.childGroups();

		for (int j = 0; j < keys.count(); ++j)
		{
			SettingsManager::setDefaultValue(QString("%1/%2").arg(groups.at(i)).arg(keys.at(j)), defaults.value(QString("%1/value").arg(keys.at(j))));
		}

		defaults.endGroup();
	}

	SettingsManager::setDefaultValue("Paths/Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
	SettingsManager::setDefaultValue("Paths/SaveFile", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
	SettingsManager::setDefaultValue("Actions/NewTab", "Ctrl+T");
	SettingsManager::setDefaultValue("Actions/NewWindow", QKeySequence(QKeySequence::New).toString());
	SettingsManager::setDefaultValue("Actions/Open", QKeySequence(QKeySequence::Open).toString());
	SettingsManager::setDefaultValue("Actions/Save", QKeySequence(QKeySequence::Save).toString());
	SettingsManager::setDefaultValue("Actions/Exit", QKeySequence(QKeySequence::Quit).toString());
	SettingsManager::setDefaultValue("Actions/Undo", QKeySequence(QKeySequence::Undo).toString());
	SettingsManager::setDefaultValue("Actions/Redo", QKeySequence(QKeySequence::Redo).toString());
	SettingsManager::setDefaultValue("Actions/Redo", QKeySequence(QKeySequence::Redo).toString());
	SettingsManager::setDefaultValue("Actions/Cut", QKeySequence(QKeySequence::Cut).toString());
	SettingsManager::setDefaultValue("Actions/Copy", QKeySequence(QKeySequence::Copy).toString());
	SettingsManager::setDefaultValue("Actions/Paste", QKeySequence(QKeySequence::Paste).toString());
	SettingsManager::setDefaultValue("Actions/Delete", QKeySequence(QKeySequence::Delete).toString());
	SettingsManager::setDefaultValue("Actions/SelectAll", QKeySequence(QKeySequence::SelectAll).toString());
	SettingsManager::setDefaultValue("Actions/Find", QKeySequence(QKeySequence::Find).toString());
	SettingsManager::setDefaultValue("Actions/FindNext", QKeySequence(QKeySequence::FindNext).toString());
	SettingsManager::setDefaultValue("Actions/FindPrevious", QKeySequence(QKeySequence::FindPrevious).toString());
	SettingsManager::setDefaultValue("Actions/Reload", QKeySequence(QKeySequence::Refresh).toString());
	SettingsManager::setDefaultValue("Actions/ZoomIn", QKeySequence(QKeySequence::ZoomIn).toString());
	SettingsManager::setDefaultValue("Actions/ZoomOut", QKeySequence(QKeySequence::ZoomOut).toString());
	SettingsManager::setDefaultValue("Actions/Back", QKeySequence(QKeySequence::Back).toString());
	SettingsManager::setDefaultValue("Actions/Forward", QKeySequence(QKeySequence::Forward).toString());
	SettingsManager::setDefaultValue("Actions/Help", QKeySequence(QKeySequence::HelpContents).toString());
	SettingsManager::setDefaultValue("Actions/ApplicationConfiguration", QKeySequence(QKeySequence::Preferences).toString());
	SettingsManager::setDefaultValue("Actions/Fullscreen", QKeySequence("F11").toString());

	ActionsManager::createInstance(this);

	BookmarksManager::createInstance(this);

	HistoryManager::createInstance(this);

	WebBackendsManager::createInstance(this);

	SearchesManager::createInstance(this);

	SessionsManager::createInstance(this);

	TransfersManager::createInstance(this);

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));

	QTranslator applicationTranslator;
	applicationTranslator.load(":/translations/otter_" + QLocale::system().name());

	installTranslator(&qtTranslator);
	installTranslator(&applicationTranslator);
	setQuitOnLastWindowClosed(true);
}

Application::~Application()
{
	for (int i = 0; i < m_windows.size(); ++i)
	{
		m_windows.at(i)->deleteLater();
	}
}

void Application::removeWindow(MainWindow *window)
{
	m_windows.removeAll(window);

	window->deleteLater();
}

void Application::newConnection()
{
	QLocalSocket *socket = m_localServer->nextPendingConnection();

	if (!socket)
	{
		return;
	}

	socket->waitForReadyRead(1000);

	MainWindow *window = NULL;
	QString data;
	QStringList arguments;
	QTextStream stream(socket);
	stream >> data;

	QByteArray byteArray = QByteArray::fromBase64(data.toUtf8());
	QDataStream in(&byteArray, QIODevice::ReadOnly);
	in >> arguments;

	QCommandLineParser *parser = getParser();
	parser->parse(arguments);

	if (!SettingsManager::getValue("Browser/OpenLinksInNewTab").toBool() && !parser->isSet("privatesession"))
	{
		window = createWindow(parser->isSet("privatesession"));
	}
	else
	{
		window = getWindow();
	}

	if (window)
	{
		if (parser->positionalArguments().isEmpty())
		{
			window->openUrl();
		}
		else
		{
			QStringList urls = parser->positionalArguments();

			for (int i = 0; i < urls.count(); ++i)
			{
				window->openUrl(QUrl(urls.at(i)));
			}
		}
	}

	delete socket;

	if (window)
	{
		window->raise();
		window->activateWindow();
	}

	delete parser;
}

void Application::newWindow(bool privateSession, bool background, const QUrl &url)
{
	MainWindow *window = createWindow(privateSession, background);

	if (url.isValid() && window)
	{
		window->openUrl(url);
	}
}

MainWindow* Application::createWindow(bool privateSession, bool background, const SessionEntry &windows)
{
	MainWindow *window = new MainWindow(privateSession, windows);

	m_windows.prepend(window);

	if (background)
	{
		window->setAttribute(Qt::WA_ShowWithoutActivating, true);
	}

	window->show();

	if (background)
	{
		window->lower();
		window->setAttribute(Qt::WA_ShowWithoutActivating, false);
	}

	connect(window, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SLOT(newWindow(bool,bool,QUrl)));

	return window;
}

MainWindow* Application::getWindow()
{
	if (m_windows.isEmpty())
	{
		return createWindow();
	}

	return m_windows[0];
}

QList<MainWindow*> Application::getWindows()
{
	return m_windows;
}

QCommandLineParser* Application::getParser() const
{
	QCommandLineParser *parser = new QCommandLineParser();
	parser->addHelpOption();
	parser->addVersionOption();
	parser->addPositionalArgument("url", QCoreApplication::translate("main", "URL to open."), "[url]");
	parser->addOption(QCommandLineOption("path", QCoreApplication::translate("main", "Uses <path> as profile directory."), "path", QString()));
	parser->addOption(QCommandLineOption("session", QCoreApplication::translate("main", "Restores session <session> if it exists."), "session", QString()));
	parser->addOption(QCommandLineOption("privatesession", QCoreApplication::translate("main", "Starts private session.")));

	return parser;
}

bool Application::isRunning() const
{
	return (m_localServer == NULL);
}

}
