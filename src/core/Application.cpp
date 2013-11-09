#include "Application.h"
#include "SettingsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QUrl>
#include <QtNetwork/QLocalSocket>

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

	SettingsManager::createInstance(this);
	SettingsManager::setDefaultValue("General/OpenLinksInNewWindow", false);

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

	if (!url.isEmpty())
	{
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
