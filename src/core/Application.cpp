#include "Application.h"
#include "ActionsManager.h"
#include "BookmarksManager.h"
#include "SettingsManager.h"
#include "../backends/web/WebBackendsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>
#include <QtCore/QStandardPaths>
#include <QtCore/QTranslator>
#include <QtNetwork/QLocalSocket>

namespace Otter
{

Application::Application(int &argc, char **argv) : QApplication(argc, argv),
	m_localServer(NULL)
{
	setApplicationName("Otter");
	setApplicationVersion("0.0.01");

	QLocalSocket socket;
	socket.connectToServer(applicationName());

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

	if (!m_localServer->listen(applicationName()))
	{
		if (m_localServer->serverError() == QAbstractSocket::AddressInUseError && QFile::exists(m_localServer->serverName()))
		{
			QFile::remove(m_localServer->serverName());

			m_localServer->listen(applicationName());
		}
	}

	SettingsManager::createInstance(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/otter/otter.conf", this);
	SettingsManager::setDefaultValue("Browser/OpenLinksInNewWindow", false);
	SettingsManager::setDefaultValue("Browser/EnablePlugins", true);
	SettingsManager::setDefaultValue("Browser/EnableJava", true);
	SettingsManager::setDefaultValue("Browser/EnableJavaScript", true);
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
	SettingsManager::setDefaultValue("Actions/Find", QKeySequence(QKeySequence::Find).toString() + ",/");
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

	WebBackendsManager::createInstance(this);

	SessionsManager::createInstance(this);

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

	if (SettingsManager::getValue("Browser/OpenLinksInNewWindow").toBool() && !parser->isSet("privatesession"))
	{
		window = createWindow(parser->isSet("privatesession"));
	}
	else
	{
		window = getWindow();
	}

	if (window && !parser->positionalArguments().isEmpty())
	{
		QStringList urls = parser->positionalArguments();

		for (int i = 0; i < urls.count(); ++i)
		{
			window->openUrl(QUrl(urls.at(i)));
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
	parser->addOption(QCommandLineOption("session", QCoreApplication::translate("main", "Restores session <session> if it exists."), "session", QString()));
	parser->addOption(QCommandLineOption("privatesession", QCoreApplication::translate("main", "Starts private session.")));

	return parser;
}

bool Application::isRunning() const
{
	return (m_localServer == NULL);
}

}
