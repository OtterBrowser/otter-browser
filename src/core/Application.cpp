/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "Application.h"
#include "AddonsManager.h"
#include "BookmarksManager.h"
#include "Console.h"
#include "HistoryManager.h"
#include "NetworkManagerFactory.h"
#include "NotesManager.h"
#include "PlatformIntegration.h"
#include "SearchesManager.h"
#include "SettingsManager.h"
#include "ToolBarsManager.h"
#include "Transfer.h"
#include "TransfersManager.h"
#include "./config.h"
#ifdef Q_OS_WIN
#include "../modules/platforms/windows/WindowsPlatformIntegration.h"
#endif
#include "../ui/MainWindow.h"
#include "../ui/NotificationDialog.h"
#include "../ui/TrayIcon.h"

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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

Application* Application::m_instance = NULL;

Application::Application(int &argc, char **argv) : QApplication(argc, argv),
	m_platformIntegration(NULL),
	m_trayIcon(NULL),
	m_qtTranslator(NULL),
	m_applicationTranslator(NULL),
	m_localServer(NULL),
	m_isHidden(false)
{
	setApplicationName(QLatin1String("Otter"));
	setApplicationVersion(OTTER_VERSION_MAIN);
	setWindowIcon(QIcon::fromTheme(QLatin1String("otter-browser"), QIcon(QLatin1String(":/icons/otter-browser.png"))));

	m_instance = this;

	QString profilePath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/otter");
	QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

	m_localePath = OTTER_INSTALL_PREFIX + QLatin1String("/share/otter-browser/locale/");

	if (QFile::exists(applicationDirPath() + QLatin1String("/locale/")))
	{
		m_localePath = applicationDirPath() + QLatin1String("/locale/");
	}

	setLocale(QLatin1String("system"));

	QCommandLineParser *parser = createCommandLineParser();
	parser->process(arguments());

	const bool isPortable = parser->isSet(QLatin1String("portable"));
	const bool isPrivate = parser->isSet(QLatin1String("privatesession"));

	if (isPortable)
	{
		profilePath = applicationDirPath() + QLatin1String("/profile");
		cachePath = applicationDirPath() + QLatin1String("/cache");
	}

	if (parser->isSet(QLatin1String("profile")))
	{
		profilePath = parser->value(QLatin1String("profile"));

		if (!profilePath.contains(QDir::separator()))
		{
			profilePath = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/otter/profiles/")).absoluteFilePath(profilePath);
		}
	}

	profilePath = QFileInfo(profilePath).absoluteFilePath();

	if (parser->isSet(QLatin1String("cache")))
	{
		cachePath = parser->value(QLatin1String("cache"));
	}

	cachePath = QFileInfo(cachePath).absoluteFilePath();

	delete parser;

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(profilePath.toUtf8());

	const QString identifier(hash.result().toHex());
	const QString server = applicationName() + (identifier.isEmpty() ? QString() : (QLatin1Char('-') + identifier));
	QLocalSocket socket;
	socket.connectToServer(server);

	if (socket.waitForConnected(500))
	{
		const QStringList decodedArguments = arguments();
		QStringList encodedArguments;

#ifdef Q_OS_WIN
		AllowSetForegroundWindow(ASFW_ANY);
#endif

		for (int i = 0; i < decodedArguments.count(); ++i)
		{
			encodedArguments.append(decodedArguments.at(i).toUtf8().toBase64());
		}

		QTextStream stream(&socket);
		stream << encodedArguments.join(QLatin1Char(' ')).toUtf8().toBase64();
		stream.flush();

		socket.waitForBytesWritten();

		return;
	}

	m_localServer = new QLocalServer(this);

	connect(m_localServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

	if (!m_localServer->listen(server) && m_localServer->serverError() == QAbstractSocket::AddressInUseError && QLocalServer::removeServer(server))
	{
		m_localServer->listen(server);
	}

	if (!QFile::exists(profilePath))
	{
		QDir().mkpath(profilePath);
	}

	Console::createInstance(this);

	SettingsManager::createInstance(profilePath, this);

	QSettings defaults(QLatin1String(":/schemas/options.ini"), QSettings::IniFormat, this);
	const QStringList groups = defaults.childGroups();

	for (int i = 0; i < groups.count(); ++i)
	{
		defaults.beginGroup(groups.at(i));

		const QStringList keys = defaults.childGroups();

		for (int j = 0; j < keys.count(); ++j)
		{
			SettingsManager::setDefaultValue(QStringLiteral("%1/%2").arg(groups.at(i)).arg(keys.at(j)), defaults.value(QStringLiteral("%1/value").arg(keys.at(j))));
		}

		defaults.endGroup();
	}

	SettingsManager::setDefaultValue(QLatin1String("Paths/Downloads"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
	SettingsManager::setDefaultValue(QLatin1String("Paths/SaveFile"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

	SessionsManager::createInstance(profilePath, cachePath, isPrivate, this);

	NetworkManagerFactory::createInstance(this);

	AddonsManager::createInstance(this);

	BookmarksManager::createInstance(this);

	HistoryManager::createInstance(this);

	NotesManager::createInstance(this);

	SearchesManager::createInstance(this);

	ToolBarsManager::createInstance(this);

	TransfersManager::createInstance(this);

	setLocale(SettingsManager::getValue(QLatin1String("Browser/Locale")).toString());
	setQuitOnLastWindowClosed(true);

	if (SettingsManager::getValue(QLatin1String("Browser/EnableTrayIcon")).toBool())
	{
		m_trayIcon = new TrayIcon(this);
	}

#ifdef Q_OS_WIN
	m_platformIntegration = new WindowsPlatformIntegration(this);
#endif

	connect(this, SIGNAL(aboutToQuit()), this, SLOT(clearHistory()));
}

Application::~Application()
{
	for (int i = 0; i < m_windows.size(); ++i)
	{
		m_windows.at(i)->deleteLater();
	}
}

void Application::close()
{
	if (canClose())
	{
		SessionsManager::saveSession();

		exit();
	}
}

void Application::removeWindow(MainWindow *window)
{
	m_windows.removeAll(window);

	window->deleteLater();

	emit windowRemoved(window);
}

void Application::showNotification(Notification *notification)
{
	if (SettingsManager::getValue(QLatin1String("Interface/UseNativeNotification")).toBool() && m_platformIntegration && m_platformIntegration->canShowNotifications())
	{
		m_platformIntegration->showNotification(notification);
	}
	else
	{
		NotificationDialog *dialog = new NotificationDialog(notification);
		dialog->show();
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

	MainWindow *window = (getWindows().isEmpty() ? NULL : getWindow());
	QString data;
	QTextStream stream(socket);
	stream >> data;

	const QStringList encodedArguments = QString(QByteArray::fromBase64(data.toUtf8())).split(QLatin1Char(' '));
	QStringList decodedArguments;

	for (int i = 0; i < encodedArguments.count(); ++i)
	{
		decodedArguments.append(QString(QByteArray::fromBase64(encodedArguments.at(i).toUtf8())));
	}

	QCommandLineParser *parser = createCommandLineParser();
	parser->parse(decodedArguments);

	const QString session = parser->value(QLatin1String("session"));
	const bool isPrivate = parser->isSet(QLatin1String("privatesession"));

	if (session.isEmpty())
	{
		if (!window || !SettingsManager::getValue(QLatin1String("Browser/OpenLinksInNewTab")).toBool() || (isPrivate && !window->getWindowsManager()->isPrivate()))
		{
			window = createWindow(isPrivate);
		}
	}
	else
	{
		const SessionInformation sessionData = SessionsManager::getSession(session);

		if (sessionData.clean || QMessageBox::warning(NULL, tr("Warning"), tr("This session was not saved correctly.\nAre you sure that you want to restore this session anyway?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			for (int i = 0; i < sessionData.windows.count(); ++i)
			{
				createWindow(isPrivate, false, sessionData.windows.at(i));
			}
		}
	}

	if (window)
	{
		if (parser->positionalArguments().isEmpty())
		{
			window->triggerAction(Action::NewTabAction);
		}
		else
		{
			const QStringList urls = parser->positionalArguments();

			for (int i = 0; i < urls.count(); ++i)
			{
				window->openUrl(urls.at(i));
			}
		}
	}

	delete socket;

	if (window)
	{
		window->raise();
		window->activateWindow();

		if (m_isHidden)
		{
			setHidden(false);
		}
		else
		{
			window->storeWindowState();
			window->restoreWindowState();
		}
	}

	delete parser;
}

void Application::clearHistory()
{
	QStringList clearSettings = SettingsManager::getValue(QLatin1String("History/ClearOnClose")).toStringList();
	clearSettings.removeAll(QString());

	if (!clearSettings.isEmpty())
	{
		if (clearSettings.contains(QLatin1String("browsing")))
		{
			HistoryManager::clearHistory();
		}

		if (clearSettings.contains(QLatin1String("cookies")))
		{
			NetworkManagerFactory::clearCookies();
		}

		if (clearSettings.contains(QLatin1String("downloads")))
		{
			TransfersManager::clearTransfers();
		}

		if (clearSettings.contains(QLatin1String("cache")))
		{
			NetworkManagerFactory::clearCache();
		}
	}
}

void Application::newWindow(bool isPrivate, bool inBackground, const QUrl &url)
{
	MainWindow *window = createWindow(isPrivate, inBackground);

	if (url.isValid() && window)
	{
		window->openUrl(url.toString());
	}
}

void Application::setHidden(bool hidden)
{
	if (hidden == m_isHidden)
	{
		return;
	}

	m_isHidden = hidden;

	const QList<MainWindow*> windows = SessionsManager::getWindows();

	for (int i = 0; i < windows.count(); ++i)
	{
		if (!windows.at(i))
		{
			continue;
		}

		if (m_isHidden)
		{
			windows.at(i)->storeWindowState();
			windows.at(i)->hide();
		}
		else
		{
			windows.at(i)->show();
			windows.at(i)->activateWindow();
			windows.at(i)->restoreWindowState();
		}
	}
}

void Application::setLocale(const QString &locale)
{
	if (!m_qtTranslator)
	{
		m_qtTranslator = new QTranslator(this);
		m_applicationTranslator = new QTranslator(this);

		installTranslator(m_qtTranslator);
		installTranslator(m_applicationTranslator);
	}

	const QString identifier = (locale.endsWith(QLatin1String(".qm")) ? QFileInfo(locale).baseName().remove(QLatin1String("otter-browser_")) : ((locale == QLatin1String("system")) ? QLocale::system().name() : locale));

	m_qtTranslator->load(QLatin1String("qt_") + ((locale.isEmpty() || locale == QLatin1String("system")) ? QLocale::system().name() : identifier), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	m_applicationTranslator->load((locale.endsWith(QLatin1String(".qm")) ? locale : QLatin1String("otter-browser_") + ((locale.isEmpty() || locale == QLatin1String("system")) ? QLocale::system().name() : locale)), m_localePath);

	QLocale::setDefault(QLocale(identifier));
}

QCommandLineParser* Application::createCommandLineParser() const
{
	QCommandLineParser *parser = new QCommandLineParser();
	parser->addHelpOption();
	parser->addVersionOption();
	parser->addPositionalArgument(QLatin1String("url"), QCoreApplication::translate("main", "URL to open"), QLatin1String("[url]"));
	parser->addOption(QCommandLineOption(QLatin1String("cache"), QCoreApplication::translate("main", "Uses <path> as cache directory"), QLatin1String("path"), QString()));
	parser->addOption(QCommandLineOption(QLatin1String("profile"), QCoreApplication::translate("main", "Uses <path> as profile directory"), QLatin1String("path"), QString()));
	parser->addOption(QCommandLineOption(QLatin1String("session"), QCoreApplication::translate("main", "Restores session <session> if it exists"), QLatin1String("session"), QString()));
	parser->addOption(QCommandLineOption(QLatin1String("privatesession"), QCoreApplication::translate("main", "Starts private session")));
	parser->addOption(QCommandLineOption(QLatin1String("sessionchooser"), QCoreApplication::translate("main", "Forces session chooser dialog")));
	parser->addOption(QCommandLineOption(QLatin1String("portable"), QCoreApplication::translate("main", "Sets profile and cache paths to directories inside the same directory as that of application binary")));

	return parser;
}

MainWindow* Application::createWindow(bool isPrivate, bool inBackground, const SessionMainWindow &windows)
{
	MainWindow *window = new MainWindow(isPrivate, windows);

	m_windows.prepend(window);

	if (inBackground)
	{
		window->setAttribute(Qt::WA_ShowWithoutActivating, true);
	}

	window->show();

	if (inBackground)
	{
		window->lower();
		window->setAttribute(Qt::WA_ShowWithoutActivating, false);
	}

	connect(window, SIGNAL(requestedNewWindow(bool,bool,QUrl)), this, SLOT(newWindow(bool,bool,QUrl)));

	emit windowAdded(window);

	return window;
}

Application* Application::getInstance()
{
	return m_instance;
}

MainWindow* Application::getWindow()
{
	if (m_windows.isEmpty())
	{
		return createWindow();
	}

	return m_windows[0];
}

PlatformIntegration* Application::getPlatformIntegration()
{
	return m_platformIntegration;
}

TrayIcon* Application::getTrayIcon()
{
	return m_trayIcon;
}

QString Application::getFullVersion() const
{
	return QStringLiteral("%1%2").arg(OTTER_VERSION_MAIN).arg(OTTER_VERSION_CONTEXT);
}

QString Application::getLocalePath() const
{
	return m_localePath;
}

QList<MainWindow*> Application::getWindows() const
{
	return m_windows;
}

bool Application::canClose()
{
	const QList<Transfer*> transfers = TransfersManager::getTransfers();
	int runningTransfers = 0;
	bool transfersDialog = false;

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers.at(i)->getState() == Transfer::RunningState)
		{
			++runningTransfers;
		}
	}

	if (runningTransfers > 0 && SettingsManager::getValue(QLatin1String("Choices/WarnQuitTransfers")).toBool())
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You are about to quit while %n files are still being downloaded.", "", runningTransfers));
		messageBox.setInformativeText(tr("Do you want to continue?"));
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Cancel);
		messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

		QPushButton *hideButton = messageBox.addButton(tr("Hide"), QMessageBox::ActionRole);
		const int result = messageBox.exec();

		SettingsManager::setValue(QLatin1String("Choices/WarnQuitTransfers"), !messageBox.checkBox()->isChecked());

		if (messageBox.clickedButton() == hideButton)
		{
			setHidden(true);

			return false;
		}

		if (result == QMessageBox::Yes)
		{
			runningTransfers = 0;
			transfersDialog = true;
		}

		if (runningTransfers > 0)
		{
			return false;
		}
	}

	const QString warnQuitMode = SettingsManager::getValue(QLatin1String("Choices/WarnQuit")).toString();

	if (!transfersDialog && warnQuitMode != QLatin1String("noWarn"))
	{
		int tabsAmount = 0;

		for (int i = 0; i < m_windows.count(); ++i)
		{
			if (m_windows.at(i))
			{
				tabsAmount += m_windows.at(i)->getWindowsManager()->getWindowCount();
			}
		}

		if (warnQuitMode == QLatin1String("alwaysWarn") || (tabsAmount > 1 && warnQuitMode == QLatin1String("warnOpenTabs")))
		{
			QMessageBox messageBox;
			messageBox.setWindowTitle(tr("Question"));
			messageBox.setText(tr("You are about to quit the current Otter Browser session."));
			messageBox.setInformativeText(tr("Do you want to continue?"));
			messageBox.setIcon(QMessageBox::Question);
			messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			messageBox.setDefaultButton(QMessageBox::Yes);
			messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

			QPushButton *hideButton = messageBox.addButton(tr("Hide"), QMessageBox::ActionRole);
			const int result = messageBox.exec();

			if (messageBox.checkBox()->isChecked())
			{
				SettingsManager::setValue(QLatin1String("Choices/WarnQuit"), QLatin1String("noWarn"));
			}

			if (result == QMessageBox::Cancel)
			{
				return false;
			}

			if (messageBox.clickedButton() == hideButton)
			{
				setHidden(true);

				return false;
			}
		}
	}

	return true;
}

bool Application::isHidden() const
{
	return m_isHidden;
}

bool Application::isRunning() const
{
	return (m_localServer == NULL);
}

}
