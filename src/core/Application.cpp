#include "Application.h"
#include "ActionsManager.h"
#include "SettingsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>
#include <QtCore/QStandardPaths>
#include <QtCore/QTranslator>
#include <QtCore/QUrl>
#include <QtNetwork/QLocalSocket>
#include <QtWebKit/QWebSettings>

namespace Otter
{

Application::Application(int &argc, char **argv) : QApplication(argc, argv),
	m_localServer(NULL)
{
	setApplicationName("otter");
	setApplicationVersion("0.0.01");
	setOrganizationName("eSoftware");
	setOrganizationDomain("otter.emdek.pl");

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

	const QString iconsPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/icons";

	QDir().mkpath(iconsPath);

	SettingsManager::createInstance(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/otter/otter.conf", this);
	SettingsManager::setDefaultValue("Browser/OpenLinksInNewWindow", false);
	SettingsManager::setDefaultValue("Browser/EnablePlugins", true);
	SettingsManager::setDefaultValue("Browser/EnableJava", true);
	SettingsManager::setDefaultValue("Browser/EnableJavaScript", true);
	SettingsManager::setDefaultValue("Actions/NewTab", QVariant(QKeySequence(QKeySequence::New).toString()));
	SettingsManager::setDefaultValue("Actions/Open", QVariant(QKeySequence(QKeySequence::Open).toString()));
	SettingsManager::setDefaultValue("Actions/Save", QVariant(QKeySequence(QKeySequence::Save).toString()));
	SettingsManager::setDefaultValue("Actions/Exit", QVariant(QKeySequence(QKeySequence::Quit).toString()));
	SettingsManager::setDefaultValue("Actions/Undo", QVariant(QKeySequence(QKeySequence::Undo).toString()));
	SettingsManager::setDefaultValue("Actions/Redo", QVariant(QKeySequence(QKeySequence::Redo).toString()));
	SettingsManager::setDefaultValue("Actions/Redo", QVariant(QKeySequence(QKeySequence::Redo).toString()));
	SettingsManager::setDefaultValue("Actions/Cut", QVariant(QKeySequence(QKeySequence::Cut).toString()));
	SettingsManager::setDefaultValue("Actions/Copy", QVariant(QKeySequence(QKeySequence::Copy).toString()));
	SettingsManager::setDefaultValue("Actions/Paste", QVariant(QKeySequence(QKeySequence::Paste).toString()));
	SettingsManager::setDefaultValue("Actions/Delete", QVariant(QKeySequence(QKeySequence::Delete).toString()));
	SettingsManager::setDefaultValue("Actions/SelectAll", QVariant(QKeySequence(QKeySequence::SelectAll).toString()));
	SettingsManager::setDefaultValue("Actions/Reload", QVariant(QKeySequence(QKeySequence::Refresh).toString()));
	SettingsManager::setDefaultValue("Actions/ZoomIn", QVariant(QKeySequence(QKeySequence::ZoomIn).toString()));
	SettingsManager::setDefaultValue("Actions/ZoomOut", QVariant(QKeySequence(QKeySequence::ZoomOut).toString()));
	SettingsManager::setDefaultValue("Actions/Back", QVariant(QKeySequence(QKeySequence::Back).toString()));
	SettingsManager::setDefaultValue("Actions/Forward", QVariant(QKeySequence(QKeySequence::Forward).toString()));
	SettingsManager::setDefaultValue("Actions/Help", QVariant(QKeySequence(QKeySequence::HelpContents).toString()));
	SettingsManager::setDefaultValue("Actions/ApplicationConfiguration", QVariant(QKeySequence(QKeySequence::Preferences).toString()));
	SettingsManager::setDefaultValue("Actions/Fullscreen", QVariant(QKeySequence("F11").toString()));

	ActionsManager::createInstance(this);

	QWebSettings *globalSettings = QWebSettings::globalSettings();
	globalSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	globalSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	globalSettings->setAttribute(QWebSettings::PluginsEnabled, SettingsManager::getValue("Browser/EnablePlugins").toBool());
	globalSettings->setAttribute(QWebSettings::JavaEnabled, SettingsManager::getValue("Browser/EnableJava").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getValue("Browser/EnableJavaScript").toBool());
	globalSettings->setIconDatabasePath(iconsPath);

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
		MainWindow *window = m_windows.at(i);

		delete window;
	}
}

void Application::cleanup()
{
	for (int i = m_windows.count() - 1; i >= 0; --i)
	{
		if (m_windows.at(i).isNull())
		{
			m_windows.removeAt(i);
		}
	}
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

void Application::newWindow()
{
	createWindow(false);
}

void Application::newWindowPrivate()
{
	createWindow(true);
}

MainWindow* Application::createWindow(bool privateSession)
{
	MainWindow *window = new MainWindow(privateSession);

	m_windows.prepend(window);

	window->show();

	connect(window, SIGNAL(requestedNewWindow()), this, SLOT(newWindow()));
	connect(window, SIGNAL(requestedNewWindowPrivate()), this, SLOT(newWindowPrivate()));

	return window;
}

MainWindow *Application::getWindow()
{
	cleanup();

	if (m_windows.isEmpty())
	{
		return createWindow();
	}

	return m_windows[0];
}

QCommandLineParser* Application::getParser() const
{
	QCommandLineParser *parser = new QCommandLineParser();
	parser->addHelpOption();
	parser->addVersionOption();
	parser->addPositionalArgument("url", QCoreApplication::translate("main", "URL to open."), "[url]");
	parser->addOption(QCommandLineOption("privatesession", QCoreApplication::translate("main", "Starts private session.")));

	return parser;
}

bool Application::isRunning() const
{
	return (m_localServer == NULL);
}

}
