#include "Application.h"
#include "ActionsManager.h"
#include "SettingsManager.h"
#include "../ui/MainWindow.h"

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
		QTextStream stream(&socket);
		QStringList args = arguments();

		if (args.count() > 1)
		{
			stream << args.last();
		}
		else
		{
			stream << QString();
		}

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

	SettingsManager::createInstance(this);
	SettingsManager::setDefaultValue("General/OpenLinksInNewWindow", false);
	SettingsManager::setDefaultValue("General/EnablePlugins", true);
	SettingsManager::setDefaultValue("General/EnableJava", true);
	SettingsManager::setDefaultValue("General/EnableJavaScript", true);
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
	globalSettings->setAttribute(QWebSettings::PluginsEnabled, SettingsManager::getValue("General/EnablePlugins").toBool());
	globalSettings->setAttribute(QWebSettings::JavaEnabled, SettingsManager::getValue("General/EnableJava").toBool());
	globalSettings->setAttribute(QWebSettings::JavascriptEnabled, SettingsManager::getValue("General/EnableJavaScript").toBool());
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
	QString url;
	QTextStream stream(socket);
	stream >> url;

	if (SettingsManager::getValue("General/OpenLinksInNewWindow").toBool())
	{
		window = newWindow();
	}
	else
	{
		window = getWindow();
	}

	if (window)
	{
		window->openUrl(QUrl(url));
	}

	delete socket;

	if (window)
	{
		window->raise();
		window->activateWindow();
	}
}

MainWindow* Application::newWindow()
{
	MainWindow *window = new MainWindow();

	m_windows.prepend(window);

	window->show();

	return window;
}

MainWindow *Application::getWindow()
{
	cleanup();

	if (m_windows.isEmpty())
	{
		return newWindow();
	}

	return m_windows[0];
}

bool Application::isRunning() const
{
	return (m_localServer == NULL);
}

}
