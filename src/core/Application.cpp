/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "Application.h"
#include "AddonsManager.h"
#include "BookmarksManager.h"
#include "Console.h"
#include "GesturesManager.h"
#include "HandlersManager.h"
#include "HistoryManager.h"
#include "LongTermTimer.h"
#include "Migrator.h"
#include "NetworkManagerFactory.h"
#include "NotesManager.h"
#include "NotificationsManager.h"
#include "PasswordsManager.h"
#include "PlatformIntegration.h"
#include "SearchEnginesManager.h"
#include "SettingsManager.h"
#include "SpellCheckManager.h"
#include "ToolBarsManager.h"
#include "ThemesManager.h"
#include "TransfersManager.h"
#include "Utils.h"
#include "Updater.h"
#include "WebBackend.h"
#ifdef Q_OS_WIN
#include "../modules/platforms/windows/WindowsPlatformIntegration.h"
#elif defined(Q_OS_MAC)
#include "../modules/platforms/mac/MacPlatformIntegration.h"
#elif defined(Q_OS_UNIX)
#include "../modules/platforms/freedesktoporg/FreeDesktopOrgPlatformIntegration.h"
#endif
#include "../ui/Action.h"
#include "../ui/LocaleDialog.h"
#include "../ui/MainWindow.h"
#include "../ui/NotificationDialog.h"
#include "../ui/ReportDialog.h"
#include "../ui/SaveSessionDialog.h"
#include "../ui/SessionsManagerDialog.h"
#include "../ui/Style.h"
#include "../ui/TrayIcon.h"
#include "../ui/UpdateCheckerDialog.h"
#include "../ui/Window.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QStorageInfo>
#include <QtCore/QTranslator>
#include <QtGui/QDesktopServices>
#include <QtNetwork/QLocalSocket>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

Application* Application::m_instance(nullptr);
PlatformIntegration* Application::m_platformIntegration(nullptr);
TrayIcon* Application::m_trayIcon(nullptr);
QTranslator* Application::m_qtTranslator(nullptr);
QTranslator* Application::m_applicationTranslator(nullptr);
QLocalServer* Application::m_localServer(nullptr);
QPointer<MainWindow> Application::m_activeWindow(nullptr);
QString Application::m_localePath;
QCommandLineParser Application::m_commandLineParser;
QVector<MainWindow*> Application::m_windows;
bool Application::m_isAboutToQuit(false);
bool Application::m_isHidden(false);
bool Application::m_isUpdating(false);

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
	setApplicationName(QLatin1String("Otter"));
	setApplicationDisplayName(QLatin1String("Otter Browser"));
	setApplicationVersion(OTTER_VERSION_MAIN);
	setWindowIcon(QIcon::fromTheme(QLatin1String("otter-browser"), QIcon(QLatin1String(":/icons/otter-browser.png"))));

	m_instance = this;

	QString profilePath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/otter"));
	QString cachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

	m_localePath = OTTER_INSTALL_PREFIX + QLatin1String("/share/otter-browser/locale/");

	if (QFile::exists(applicationDirPath() + QLatin1String("/locale/")))
	{
		m_localePath = applicationDirPath() + QLatin1String("/locale/");
	}

	setLocale(QLatin1String("system"));

	m_commandLineParser.addHelpOption();
	m_commandLineParser.addVersionOption();
	m_commandLineParser.addPositionalArgument(QLatin1String("url"), QCoreApplication::translate("main", "URL to open"), QLatin1String("[url]"));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("cache"), QCoreApplication::translate("main", "Uses <path> as cache directory"), QLatin1String("path"), QString()));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("profile"), QCoreApplication::translate("main", "Uses <path> as profile directory"), QLatin1String("path"), QString()));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("session"), QCoreApplication::translate("main", "Restores session <session> if it exists"), QLatin1String("session"), QString()));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("private-session"), QCoreApplication::translate("main", "Starts private session")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("session-chooser"), QCoreApplication::translate("main", "Forces session chooser dialog")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("portable"), QCoreApplication::translate("main", "Sets profile and cache paths to directories inside the same directory as that of application binary")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-tab"), QCoreApplication::translate("main", "Loads URL in new tab")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-private-tab"), QCoreApplication::translate("main", "Loads URL in new private tab")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-window"), QCoreApplication::translate("main", "Loads URL in new window")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-private-window"), QCoreApplication::translate("main", "Loads URL in new private window")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("readonly"), QCoreApplication::translate("main", "Tells application to avoid writing data to disk")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("report"), QCoreApplication::translate("main", "Prints out diagnostic report and exits application")));

	QStringList arguments(this->arguments());
	QString argumentsPath(QDir::current().filePath(QLatin1String("arguments.txt")));

	if (!QFile::exists(argumentsPath))
	{
		argumentsPath = QDir(applicationDirPath()).filePath(QLatin1String("arguments.txt"));
	}

	if (QFile::exists(argumentsPath))
	{
		QFile argumentsFile(argumentsPath);

		if (argumentsFile.open(QIODevice::ReadOnly))
		{
			QStringList temporaryArguments(QString(argumentsFile.readAll()).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts));

			if (!temporaryArguments.isEmpty())
			{
				if (arguments.isEmpty())
				{
					temporaryArguments.prepend(QFileInfo(QCoreApplication::applicationFilePath()).fileName());
				}
				else
				{
					temporaryArguments.prepend(arguments.first());
				}

				if (arguments.count() > 1)
				{
					temporaryArguments.append(arguments.mid(1));
				}

				arguments = temporaryArguments;
			}

			argumentsFile.close();
		}
	}

	m_commandLineParser.process(arguments);

	const bool isPortable(m_commandLineParser.isSet(QLatin1String("portable")));
	const bool isPrivate(m_commandLineParser.isSet(QLatin1String("private-session")));
	bool isReadOnly(m_commandLineParser.isSet(QLatin1String("readonly")));

	if (isPortable)
	{
		profilePath = applicationDirPath() + QLatin1String("/profile");
		cachePath = applicationDirPath() + QLatin1String("/cache");
	}

	if (m_commandLineParser.isSet(QLatin1String("profile")))
	{
		profilePath = m_commandLineParser.value(QLatin1String("profile"));

		if (!profilePath.contains(QDir::separator()))
		{
			profilePath = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/otter/profiles/")).absoluteFilePath(profilePath);
		}
	}

	profilePath = QDir::toNativeSeparators(QFileInfo(profilePath).absoluteFilePath());

	if (m_commandLineParser.isSet(QLatin1String("cache")))
	{
		cachePath = m_commandLineParser.value(QLatin1String("cache"));
	}

	cachePath = QDir::toNativeSeparators(QFileInfo(cachePath).absoluteFilePath());

	if (m_commandLineParser.isSet(QLatin1String("report")))
	{
		Console::createInstance();

		SettingsManager::createInstance(profilePath);

		SessionsManager::createInstance(profilePath, cachePath, isPrivate, false);

		QStringList rawReportOptions(m_commandLineParser.positionalArguments());
		ReportOptions reportOptions(BasicReport);

		if (rawReportOptions.isEmpty() || rawReportOptions.contains(QLatin1String("full")))
		{
			reportOptions = FullReport;
		}
		else
		{
			if (rawReportOptions.contains("environment"))
			{
				reportOptions |= EnvironmentReport;
			}

			if (rawReportOptions.contains("keyboardShortcuts"))
			{
				reportOptions |= KeyboardShortcutsReport;
			}

			if (rawReportOptions.contains("paths"))
			{
				reportOptions |= PathsReport;
			}

			if (rawReportOptions.contains("settings"))
			{
				reportOptions |= SettingsReport;
			}
		}

#ifdef Q_OS_WIN
		if (!rawReportOptions.contains(QLatin1String("noDialog")))
		{
			rawReportOptions.append(QLatin1String("dialog"));
		}
#endif
		if (rawReportOptions.contains(QLatin1String("dialog")))
		{
			ReportDialog dialog(reportOptions);
			dialog.exec();
		}
		else
		{
			QTextStream stream(stdout);
			stream << createReport(reportOptions);
		}

		return;
	}

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(profilePath.toUtf8());

	const QString identifier(hash.result().toHex());
	const QString server(applicationName() + (identifier.isEmpty() ? QString() : (QLatin1Char('-') + identifier)));
	QLocalSocket socket;
	socket.connectToServer(server);

	if (socket.waitForConnected(500))
	{
		const QStringList decodedArguments(arguments);
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

	connect(m_localServer, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));

	m_localServer->setSocketOptions(QLocalServer::UserAccessOption);

	if (!m_localServer->listen(server) && m_localServer->serverError() == QAbstractSocket::AddressInUseError && QLocalServer::removeServer(server))
	{
		m_localServer->listen(server);
	}

	if (!isReadOnly && !QFile::exists(profilePath))
	{
		QDir().mkpath(profilePath);
	}

	if (!isReadOnly && !QFileInfo(profilePath).isWritable())
	{
		QMessageBox::warning(nullptr, tr("Warning"), tr("Profile directory (%1) is not writable, application will be running in read-only mode.").arg(profilePath), QMessageBox::Close);

		isReadOnly = true;
	}

	Console::createInstance();

	SettingsManager::createInstance(profilePath);

	if (!isReadOnly)
	{
		const QStorageInfo storageInformation(profilePath);

		if (storageInformation.bytesAvailable() > -1 && storageInformation.bytesAvailable() < 10000000)
		{
			QString warnLowDiskSpaceMode(SettingsManager::getOption(SettingsManager::Choices_WarnLowDiskSpaceOption).toString());

			if (warnLowDiskSpaceMode == QLatin1String("warn"))
			{
				QString message;

				if (storageInformation.bytesAvailable() == 0)
				{
					message = tr("Your profile directory (%1) ran out of free disk space.\nThis may lead to malfunctions or even data loss.").arg(QDir::toNativeSeparators(profilePath));
				}
				else
				{
					message = tr("Your profile directory (%1) is running low on free disk space (%2 remaining).\nThis may lead to malfunctions or even data loss.").arg(QDir::toNativeSeparators(profilePath)).arg(Utils::formatUnit(storageInformation.bytesAvailable()));
				}

				QMessageBox messageBox;
				messageBox.setWindowTitle(tr("Warning"));
				messageBox.setText(message);
				messageBox.setInformativeText(tr("Do you want to continue?"));
				messageBox.setIcon(QMessageBox::Question);
				messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));
				messageBox.addButton(tr("Continue in Read-only Mode"), QMessageBox::YesRole);

				QAbstractButton *ignoreButton(messageBox.addButton(tr("Ignore"), QMessageBox::ActionRole));
				QAbstractButton *quitButton(messageBox.addButton(tr("Quit"), QMessageBox::RejectRole));

				messageBox.exec();

				if (messageBox.clickedButton() == quitButton)
				{
					quit();

					return;
				}

				warnLowDiskSpaceMode = ((messageBox.clickedButton() == ignoreButton) ? QLatin1String("continueReadWrite") : QLatin1String("continueReadOnly"));

				if (messageBox.checkBox()->isChecked())
				{
					SettingsManager::setOption(SettingsManager::Choices_WarnLowDiskSpaceOption, warnLowDiskSpaceMode);
				}
			}

			if (warnLowDiskSpaceMode == QLatin1String("continueReadOnly"))
			{
				isReadOnly = true;
			}
		}
	}

	SessionsManager::createInstance(profilePath, cachePath, isPrivate, isReadOnly);

	if (!isReadOnly && !Migrator::run())
	{
		m_isAboutToQuit = true;

		exit();

		return;
	}

	ThemesManager::createInstance();

	ActionsManager::createInstance();

	AddonsManager::createInstance();

	BookmarksManager::createInstance();

	GesturesManager::createInstance();

	HandlersManager::createInstance();

	HistoryManager::createInstance();

	NetworkManagerFactory::createInstance();

	NotesManager::createInstance();

	NotificationsManager::createInstance();

	PasswordsManager::createInstance();

	SearchEnginesManager::createInstance();

	SpellCheckManager::createInstance();

	ToolBarsManager::createInstance();

	TransfersManager::createInstance();

	setLocale(SettingsManager::getOption(SettingsManager::Browser_LocaleOption).toString());
	setQuitOnLastWindowClosed(true);

	const WebBackend *webBackend(AddonsManager::getWebBackend());

	if (!QSslSocket::supportsSsl() || (webBackend && webBackend->getSslVersion().isEmpty()))
	{
		QMessageBox::warning(nullptr, tr("Warning"), tr("SSL support is not available or incomplete.\nSome websites may work incorrectly or do not work at all."), QMessageBox::Close);
	}

	if (SettingsManager::getOption(SettingsManager::Browser_EnableTrayIconOption).toBool())
	{
		m_trayIcon = new TrayIcon(this);
	}

#ifdef Q_OS_WIN
	m_platformIntegration = new WindowsPlatformIntegration(this);
#elif defined(Q_OS_MAC)
	m_platformIntegration = new MacPlatformIntegration(this);
#elif defined(Q_OS_UNIX)
	m_platformIntegration = new FreeDesktopOrgPlatformIntegration(this);
#endif

	if (Updater::isReadyToInstall())
	{
		m_isUpdating = Updater::installUpdate();

		if (m_isUpdating)
		{
			return;
		}
	}

	const QDate lastUpdate(QDate::fromString(SettingsManager::getOption(SettingsManager::Updates_LastCheckOption).toString(), Qt::ISODate));
	const int interval(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

	if (interval > 0 && (lastUpdate.isNull() ? interval : lastUpdate.daysTo(QDate::currentDate())) >= interval && !SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList().isEmpty())
	{
		UpdateChecker *updateChecker(new UpdateChecker(this));

		connect(updateChecker, SIGNAL(finished(QVector<UpdateChecker::UpdateInformation>)), this, SLOT(handleUpdateCheckResult(QVector<UpdateChecker::UpdateInformation>)));

		LongTermTimer::runTimer((interval * SECONDS_IN_DAY), this, SLOT(periodicUpdateCheck()));
	}

	Style *style(ThemesManager::createStyle(SettingsManager::getOption(SettingsManager::Interface_WidgetStyleOption).toString()));
	QString styleSheet(style->getStyleSheet());
	const QString styleSheetPath(SettingsManager::getOption(SettingsManager::Interface_StyleSheetOption).toString());

	if (!styleSheetPath.isEmpty())
	{
		QFile file(styleSheetPath);

		if (file.open(QIODevice::ReadOnly))
		{
			styleSheet += file.readAll();

			file.close();
		}
	}

	setStyle(style);
	setStyleSheet(styleSheet);

	QDesktopServices::setUrlHandler(QLatin1String("ftp"), this, "openUrl");
	QDesktopServices::setUrlHandler(QLatin1String("http"), this, "openUrl");
	QDesktopServices::setUrlHandler(QLatin1String("https"), this, "openUrl");

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
	connect(this, SIGNAL(aboutToQuit()), this, SLOT(handleAboutToQuit()));
}

Application::~Application()
{
	for (int i = 0; i < m_windows.count(); ++i)
	{
		m_windows.at(i)->deleteLater();
	}
}

void Application::triggerAction(int identifier, const QVariantMap &parameters, QObject *target)
{
	switch (identifier)
	{
		case ActionsManager::RunMacroAction:
			{
				const QVariantList actions(parameters.value(QLatin1String("actions")).toList());

				for (int i = 0; i < actions.count(); ++i)
				{
					QVariant actionIdentifier;
					QVariantMap actionParameters;

					if (actions.at(i).type() == QVariant::Map)
					{
						const QVariantMap action(actions.at(i).toMap());

						actionIdentifier = action.value(QLatin1String("identifier"));
						actionParameters = action.value(QLatin1String("parameters")).toMap();
					}
					else
					{
						actionIdentifier = actions.at(i);
					}

					if (actionIdentifier.type() == QVariant::Int)
					{
						triggerAction(actionIdentifier.toInt(), actionParameters, target);
					}
					else
					{
						triggerAction(ActionsManager::getActionIdentifier(actionIdentifier.toString()), actionParameters, target);
					}
				}
			}

			return;
		case ActionsManager::NewWindowAction:
			createWindow();

			return;
		case ActionsManager::NewWindowPrivateAction:
			createWindow({{QLatin1String("hints"), SessionsManager::PrivateOpen}});

			return;
		case ActionsManager::ClosePrivateTabsPanicAction:
			if (SessionsManager::isPrivate())
			{
				m_instance->close();
			}
			else
			{
				for (int i = 0; i < m_windows.count(); ++i)
				{
					if (m_windows[i]->isPrivate())
					{
						m_windows[i]->close();
					}
					else
					{
						m_windows[i]->triggerAction(ActionsManager::ClosePrivateTabsAction);
					}
				}
			}

			return;
		case ActionsManager::SessionsAction:
			{
				SessionsManagerDialog dialog(m_activeWindow);
				dialog.exec();
			}

			return;
		case ActionsManager::SaveSessionAction:
			{
				SaveSessionDialog dialog(m_activeWindow);
				dialog.exec();
			}

			return;
		case ActionsManager::WorkOfflineAction:
			SettingsManager::setOption(SettingsManager::Network_WorkOfflineOption, Action::calculateCheckedState(parameters));

			return;
		case ActionsManager::LockToolBarsAction:
			ToolBarsManager::setToolBarsLocked(Action::calculateCheckedState(parameters));

			return;
		case ActionsManager::ResetToolBarsAction:
			ToolBarsManager::resetToolBars();

			return;
		case ActionsManager::SwitchApplicationLanguageAction:
			{
				LocaleDialog dialog(m_activeWindow);
				dialog.exec();
			}

			return;
		case ActionsManager::CheckForUpdatesAction:
			{
				UpdateCheckerDialog *dialog(new UpdateCheckerDialog(m_activeWindow));
				dialog->setAttribute(Qt::WA_DeleteOnClose, true);
				dialog->show();
			}

			return;
		case ActionsManager::DiagnosticReportAction:
			{
				ReportDialog *dialog(new ReportDialog(Application::FullReport, m_activeWindow));
				dialog->setAttribute(Qt::WA_DeleteOnClose, true);
				dialog->show();
			}

			return;
		case ActionsManager::AboutApplicationAction:
			{
				WebBackend *webBackend(AddonsManager::getWebBackend());
				QString about = tr("<b>Otter %1</b><br>Web browser controlled by the user, not vice-versa.<br><a href=\"https://www.otter-browser.org/\">https://www.otter-browser.org/</a>").arg(Application::getFullVersion());

				if (webBackend)
				{
					const QString sslVersion(webBackend->getSslVersion());

					about.append(QLatin1String("<br><br>") + tr("Web backend: %1 %2.").arg(webBackend->getTitle()).arg(webBackend->getEngineVersion()) + QLatin1String("<br><br>"));

					if (sslVersion.isEmpty())
					{
						about.append(tr("SSL library not available."));
					}
					else
					{
						about.append(tr("SSL library version: %1.").arg(sslVersion));
					}
				}

				QMessageBox::about(m_activeWindow, QLatin1String("Otter"), about);
			}

			return;
		case ActionsManager::AboutQtAction:
			aboutQt();

			return;
		case ActionsManager::ExitAction:
			m_instance->close();

			return;
		default:
			break;
	}

	const ActionsManager::ActionDefinition::ActionScope scope(ActionsManager::getActionDefinition(identifier).scope);

	if (scope == ActionsManager::ActionDefinition::MainWindowScope || scope == ActionsManager::ActionDefinition::WindowScope)
	{
		MainWindow *mainWindow(target ? MainWindow::findMainWindow(target) : m_activeWindow.data());

		if (scope == ActionsManager::ActionDefinition::WindowScope)
		{
			Window *window(nullptr);

			if (target)
			{
				if (mainWindow && parameters.contains(QLatin1String("window")))
				{
					window = mainWindow->getWindowByIdentifier(parameters[QLatin1String("window")].toULongLong());
				}
				else
				{
					while (target)
					{
						if (target->metaObject()->className() == QLatin1String("Otter::Window"))
						{
							window = qobject_cast<Window*>(target);

							break;
						}

						target = target->parent();
					}
				}
			}

			if (!target && mainWindow)
			{
				window = mainWindow->getActiveWindow();
			}

			if (window)
			{
				window->triggerAction(identifier, parameters);
			}
		}
		else if (mainWindow)
		{
			mainWindow->triggerAction(identifier, parameters);
		}
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

void Application::openUrl(const QUrl &url)
{
	if (m_activeWindow)
	{
		m_activeWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}});
	}
}

void Application::removeWindow(MainWindow *window)
{
	m_windows.removeAll(window);

	window->deleteLater();

	emit m_instance->windowRemoved(window);

	if (m_windows.isEmpty())
	{
		exit();
	}
}

void Application::periodicUpdateCheck()
{
	UpdateChecker *updateChecker(new UpdateChecker(this));

	connect(updateChecker, SIGNAL(finished(QVector<UpdateChecker::UpdateInformation>)), this, SLOT(handleUpdateCheckResult(QVector<UpdateChecker::UpdateInformation>)));

	const int interval(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

	if (interval > 0 && !SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList().isEmpty())
	{
		LongTermTimer::runTimer((interval * SECONDS_IN_DAY), this, SLOT(periodicUpdateCheck()));
	}
}

void Application::handleOptionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Browser_EnableTrayIconOption)
	{
		if (!m_trayIcon && value.toBool())
		{
			m_trayIcon = new TrayIcon(this);
		}
		else if (m_trayIcon && !value.toBool())
		{
			m_trayIcon->deleteLater();
			m_trayIcon = nullptr;
		}
	}
}

void Application::handleAboutToQuit()
{
	m_isAboutToQuit = true;

	QStringList clearSettings(SettingsManager::getOption(SettingsManager::History_ClearOnCloseOption).toStringList());
	clearSettings.removeAll(QString());

	if (!clearSettings.isEmpty())
	{
		const bool shouldClearAll(clearSettings.contains(QLatin1String("all")));

		if (shouldClearAll || clearSettings.contains(QLatin1String("browsing")))
		{
			HistoryManager::clearHistory();
		}

		if (shouldClearAll || clearSettings.contains(QLatin1String("cookies")))
		{
			NetworkManagerFactory::clearCookies();
		}

		if (shouldClearAll || clearSettings.contains(QLatin1String("downloads")))
		{
			TransfersManager::clearTransfers();
		}

		if (shouldClearAll || clearSettings.contains(QLatin1String("caches")))
		{
			NetworkManagerFactory::clearCache();
		}

		if (shouldClearAll || clearSettings.contains(QLatin1String("passwords")))
		{
			PasswordsManager::clearPasswords();
		}
	}
}

void Application::handleNewConnection()
{
	QLocalSocket *socket(m_localServer->nextPendingConnection());

	if (!socket)
	{
		return;
	}

	socket->waitForReadyRead(1000);

	const MainWindow *window(getWindows().isEmpty() ? nullptr : getWindow());
	QString data;
	QTextStream stream(socket);
	stream >> data;

	const QStringList encodedArguments(QString(QByteArray::fromBase64(data.toUtf8())).split(QLatin1Char(' ')));
	QStringList decodedArguments;

	for (int i = 0; i < encodedArguments.count(); ++i)
	{
		decodedArguments.append(QString(QByteArray::fromBase64(encodedArguments.at(i).toUtf8())));
	}

	m_commandLineParser.parse(decodedArguments);

	const QString session(m_commandLineParser.value(QLatin1String("session")));
	const bool isPrivate(m_commandLineParser.isSet(QLatin1String("private-session")));

	if (session.isEmpty())
	{
		if (!window || !SettingsManager::getOption(SettingsManager::Browser_OpenLinksInNewTabOption).toBool() || (isPrivate && !window->isPrivate()))
		{
			QVariantMap parameters;

			if (isPrivate)
			{
				parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
			}

			createWindow(parameters);
		}
	}
	else
	{
		const SessionInformation sessionData(SessionsManager::getSession(session));

		if (sessionData.isClean || QMessageBox::warning(nullptr, tr("Warning"), tr("This session was not saved correctly.\nAre you sure that you want to restore this session anyway?"), (QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes)
		{
			QVariantMap parameters;

			if (isPrivate)
			{
				parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
			}

			for (int i = 0; i < sessionData.windows.count(); ++i)
			{
				createWindow(parameters, sessionData.windows.at(i));
			}
		}
	}

	handlePositionalArguments(&m_commandLineParser);

	delete socket;
}

void Application::handlePositionalArguments(QCommandLineParser *parser)
{
	SessionsManager::OpenHints openHints(SessionsManager::DefaultOpen);

	if (parser->isSet(QLatin1String("new-private-window")))
	{
		openHints = (SessionsManager::NewWindowOpen | SessionsManager::PrivateOpen);
	}
	else if (parser->isSet(QLatin1String("new-window")))
	{
		openHints = SessionsManager::NewWindowOpen;
	}
	else if (parser->isSet(QLatin1String("new-private-tab")))
	{
		openHints = (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen);
	}
	else if (parser->isSet(QLatin1String("new-tab")))
	{
		openHints = SessionsManager::NewTabOpen;
	}

	const QStringList urls((openHints == SessionsManager::DefaultOpen || !parser->positionalArguments().isEmpty()) ? parser->positionalArguments() : QStringList(QString()));

	if (urls.isEmpty())
	{
		return;
	}

	MainWindow *window(nullptr);

	if (openHints.testFlag(SessionsManager::NewWindowOpen))
	{
		QVariantMap parameters;

		if (openHints.testFlag(SessionsManager::PrivateOpen))
		{
			parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
		}

		for (int i = 0; i < urls.count(); ++i)
		{
			window = createWindow(parameters);

			if (!urls.at(i).isEmpty())
			{
				window->openUrl(urls.at(i));
			}
		}
	}
	else
	{
		window = getWindow();

		if (window)
		{
			for (int i = 0; i < urls.count(); ++i)
			{
				window->openUrl(urls.at(i), openHints.testFlag(SessionsManager::PrivateOpen));
			}
		}
	}

	if (window)
	{
		window->raise();
		window->activateWindow();

		if (m_isHidden)
		{
			m_instance->setHidden(false);
		}
		else
		{
			window->raiseWindow();
		}
	}
}

void Application::handleUpdateCheckResult(const QVector<UpdateChecker::UpdateInformation> &availableUpdates)
{
	if (availableUpdates.isEmpty())
	{
		return;
	}

	const int latestVersion(availableUpdates.count() - 1);

	if (SettingsManager::getOption(SettingsManager::Updates_AutomaticInstallOption).toBool())
	{
		new Updater(availableUpdates.at(latestVersion), this);
	}
	else
	{
		Notification *notification(NotificationsManager::createNotification(NotificationsManager::UpdateAvailableEvent, tr("New update %1 from %2 channel is available!").arg(availableUpdates.at(latestVersion).version).arg(availableUpdates.at(latestVersion).channel)));
		notification->setData(QVariant::fromValue<QVector<UpdateChecker::UpdateInformation> >(availableUpdates));

		connect(notification, SIGNAL(clicked()), this, SLOT(showUpdateDetails()));
	}
}

void Application::showNotification(Notification *notification)
{
	if (SettingsManager::getOption(SettingsManager::Interface_UseNativeNotificationsOption).toBool() && m_platformIntegration && m_platformIntegration->canShowNotifications())
	{
		m_platformIntegration->showNotification(notification);
	}
	else
	{
		NotificationDialog *dialog(new NotificationDialog(notification));
		dialog->show();
	}
}

void Application::showUpdateDetails()
{
	const Notification *notification(static_cast<Notification*>(sender()));

	if (notification)
	{
		UpdateCheckerDialog *dialog(new UpdateCheckerDialog(nullptr, notification->getData().value<QVector<UpdateChecker::UpdateInformation> >()));
		dialog->show();
	}
}

void Application::setHidden(bool isHidden)
{
	if (isHidden == m_isHidden)
	{
		return;
	}

	m_isHidden = isHidden;

	for (int i = 0; i < m_windows.count(); ++i)
	{
		if (m_isHidden)
		{
			m_windows.at(i)->storeWindowState();
			m_windows.at(i)->hide();
		}
		else
		{
			m_windows.at(i)->show();
			m_windows.at(i)->activateWindow();
			m_windows.at(i)->restoreWindowState();
		}
	}
}

void Application::setLocale(const QString &locale)
{
	if (!m_qtTranslator)
	{
		m_qtTranslator = new QTranslator(m_instance);
		m_applicationTranslator = new QTranslator(m_instance);

		installTranslator(m_qtTranslator);
		installTranslator(m_applicationTranslator);
	}

	const QString identifier(locale.endsWith(QLatin1String(".qm")) ? QFileInfo(locale).baseName().remove(QLatin1String("otter-browser_")) : ((locale == QLatin1String("system")) ? QLocale::system().name() : locale));

	m_qtTranslator->load(QLatin1String("qt_") + ((locale.isEmpty() || locale == QLatin1String("system")) ? QLocale::system().name() : identifier), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	m_applicationTranslator->load((locale.endsWith(QLatin1String(".qm")) ? locale : QLatin1String("otter-browser_") + ((locale.isEmpty() || locale == QLatin1String("system")) ? QLocale::system().name() : locale)), m_localePath);

	QLocale::setDefault(QLocale(identifier));
}

Action* Application::createAction(int identifier, const QVariantMap parameters, bool followState, QObject *target)
{
	MainWindow *window(MainWindow::findMainWindow(target));

	return (window ? window->createAction(identifier, parameters, followState) : nullptr);
}

MainWindow* Application::createWindow(const QVariantMap &parameters, const SessionMainWindow &windows)
{
	MainWindow *window(new MainWindow(parameters, windows));

	m_windows.prepend(window);

	const bool inBackground(SessionsManager::calculateOpenHints(parameters).testFlag(SessionsManager::BackgroundOpen));

	if (inBackground)
	{
		window->setAttribute(Qt::WA_ShowWithoutActivating, true);
	}
	else
	{
		m_activeWindow = window;
	}

	window->show();

	if (inBackground)
	{
		window->lower();
		window->setAttribute(Qt::WA_ShowWithoutActivating, false);
	}

	if (parameters.contains(QLatin1String("url")))
	{
		window->openUrl(parameters[QLatin1String("url")].toString());
	}

	emit m_instance->windowAdded(window);

	connect(window, &MainWindow::activated, [&](MainWindow *window)
	{
		m_activeWindow = window;
	});

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

MainWindow* Application::getActiveWindow()
{
	return m_activeWindow;
}

Style* Application::getStyle()
{
	return qobject_cast<Style*>(QApplication::style());
}

PlatformIntegration* Application::getPlatformIntegration()
{
	return m_platformIntegration;
}

TrayIcon* Application::getTrayIcon()
{
	return m_trayIcon;
}

QCommandLineParser* Application::getCommandLineParser()
{
	return &m_commandLineParser;
}

QString Application::createReport(ReportOptions options)
{
	ActionsManager::createInstance();
	AddonsManager::createInstance();

	const WebBackend *webBackend(AddonsManager::getWebBackend());
	QString report;
	QTextStream stream(&report);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Otter Browser diagnostic report created on ") << QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	stream << QLatin1String("\n\nVersion:\n\t");
	stream.setFieldWidth(20);
	stream << QLatin1String("Main Number");
	stream << OTTER_VERSION_MAIN;
	stream.setFieldWidth(0);
	stream << QLatin1String("\n\t");
	stream.setFieldWidth(20);
	stream << QLatin1String("Weekly Number");

	if (QString(OTTER_VERSION_WEEKLY).trimmed().isEmpty())
	{
		stream << QLatin1Char('-');
	}
	else
	{
		stream << OTTER_VERSION_WEEKLY;
	}

	stream.setFieldWidth(0);
	stream << QLatin1String("\n\t");
	stream.setFieldWidth(20);
	stream << QLatin1String("Context");
	stream << OTTER_VERSION_CONTEXT;
	stream.setFieldWidth(0);
	stream << QLatin1String("\n\t");
	stream.setFieldWidth(20);
	stream << QLatin1String("Web Backend");
	stream << (webBackend ? QStringLiteral("%1 %2").arg(webBackend->getTitle()).arg(webBackend->getEngineVersion()) : QLatin1String("none"));
	stream.setFieldWidth(0);
	stream << QLatin1String("\n\n");

	if (options.testFlag(EnvironmentReport))
	{
		stream << QLatin1String("Environment:\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("System Name");
		stream << QSysInfo::prettyProductName();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("Build ABI");
		stream << QSysInfo::buildAbi();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("Build CPU Architecture");
		stream << QSysInfo::buildCpuArchitecture();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("Current CPU Architecture");
		stream << QSysInfo::currentCpuArchitecture();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("Build Qt Version");
		stream << QT_VERSION_STR;
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("Current Qt Version");
		stream << qVersion();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(30);
		stream << QLatin1String("Locale");
		stream << QLocale::system().name();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\n");
	}

	if (options.testFlag(PathsReport))
	{
		stream << QLatin1String("Paths:\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Profile");
		stream << SessionsManager::getProfilePath();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Configuration");
		stream << SettingsManager::getGlobalPath();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Session");
		stream << SessionsManager::getSessionPath(SessionsManager::getCurrentSession());
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Overrides");
		stream << SettingsManager::getOverridePath();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Bookmarks");
		stream << SessionsManager::getWritableDataPath(QLatin1String("bookmarks.xbel"));
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Notes");
		stream << SessionsManager::getWritableDataPath(QLatin1String("notes.xbel"));
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("History");
		stream << SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.json"));
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\t");
		stream.setFieldWidth(20);
		stream << QLatin1String("Cache");
		stream << SessionsManager::getCachePath();
		stream.setFieldWidth(0);
		stream << QLatin1String("\n\n");
	}

	if (options.testFlag(SettingsReport))
	{
		stream << SettingsManager::createReport();
	}

	if (options.testFlag(KeyboardShortcutsReport))
	{
		stream << ActionsManager::createReport();
	}

	return report.remove(QRegularExpression(QLatin1String(" +$"), QRegularExpression::MultilineOption));
}

QString Application::getFullVersion()
{
	return QStringLiteral("%1%2").arg(OTTER_VERSION_MAIN).arg(OTTER_VERSION_CONTEXT);
}

QString Application::getLocalePath()
{
	return m_localePath;
}

ActionsManager::ActionDefinition::State Application::getActionState(int identifier, const QVariantMap &parameters)
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(identifier));

	switch (definition.scope)
	{
		case ActionsManager::ActionDefinition::WindowScope:
		case ActionsManager::ActionDefinition::MainWindowScope:
			if (m_activeWindow)
			{
				return m_activeWindow->getActionState(identifier, parameters);
			}

			return definition.defaultState;
		case ActionsManager::ActionDefinition::ApplicationScope:
			break;
		default:
			return definition.defaultState;
	}

	ActionsManager::ActionDefinition::State state(definition.defaultState);

	switch (identifier)
	{
		case ActionsManager::WorkOfflineAction:
			state.isChecked = SettingsManager::getOption(SettingsManager::Network_WorkOfflineOption).toBool();

			break;
		case ActionsManager::LockToolBarsAction:
			state.isChecked = ToolBarsManager::areToolBarsLocked();

			break;
		default:
			break;
	}

	return state;
}

QVector<MainWindow*> Application::getWindows()
{
	return m_windows;
}

bool Application::canClose()
{
	const QVector<Transfer*> transfers(TransfersManager::getTransfers());
	int runningTransfers(0);
	bool transfersDialog(false);

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers.at(i)->getState() == Transfer::RunningState)
		{
			++runningTransfers;
		}
	}

	if (runningTransfers > 0 && SettingsManager::getOption(SettingsManager::Choices_WarnQuitTransfersOption).toBool())
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You are about to quit while %n files are still being downloaded.", "", runningTransfers));
		messageBox.setInformativeText(tr("Do you want to continue?"));
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Cancel);
		messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

		const QPushButton *hideButton(messageBox.addButton(tr("Hide"), QMessageBox::ActionRole));
		const int result(messageBox.exec());

		SettingsManager::setOption(SettingsManager::Choices_WarnQuitTransfersOption, !messageBox.checkBox()->isChecked());

		if (messageBox.clickedButton() == hideButton)
		{
			m_instance->setHidden(true);

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

	const QString warnQuitMode(SettingsManager::getOption(SettingsManager::Choices_WarnQuitOption).toString());

	if (!transfersDialog && warnQuitMode != QLatin1String("noWarn"))
	{
		int tabsAmount(0);

		for (int i = 0; i < m_windows.count(); ++i)
		{
			if (m_windows.at(i))
			{
				tabsAmount += m_windows.at(i)->getWindowCount();
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

			const QPushButton *hideButton(messageBox.addButton(tr("Hide"), QMessageBox::ActionRole));
			const int result(messageBox.exec());

			if (messageBox.checkBox()->isChecked())
			{
				SettingsManager::setOption(SettingsManager::Choices_WarnQuitOption, QLatin1String("noWarn"));
			}

			if (result == QMessageBox::Cancel)
			{
				return false;
			}

			if (messageBox.clickedButton() == hideButton)
			{
				m_instance->setHidden(true);

				return false;
			}
		}
	}

	return true;
}

bool Application::isAboutToQuit()
{
	return m_isAboutToQuit;
}

bool Application::isHidden()
{
	return m_isHidden;
}

bool Application::isUpdating()
{
	return m_isUpdating;
}

bool Application::isRunning()
{
	return (m_localServer == nullptr);
}

}
