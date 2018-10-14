/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "FeedsManager.h"
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
QPointer<QObject> Application::m_nonMenuFocusObject(nullptr);
QString Application::m_localePath;
QCommandLineParser Application::m_commandLineParser;
QVector<MainWindow*> Application::m_windows;
bool Application::m_isAboutToQuit(false);
bool Application::m_isHidden(false);
bool Application::m_isUpdating(false);

Application::Application(int &argc, char **argv) : QApplication(argc, argv), ActionExecutor(),
	m_updateCheckTimer(nullptr)
{
	setApplicationName(QLatin1String("Otter"));
	setApplicationDisplayName(QLatin1String("Otter Browser"));
	setApplicationVersion(OTTER_VERSION_MAIN);
	setWindowIcon(QIcon::fromTheme(QLatin1String("otter-browser"), QIcon(QLatin1String(":/icons/otter-browser.png"))));

	m_instance = this;

	QString profilePath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/otter"));
	QString cachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

#ifdef Q_OS_MAC
	m_localePath = QFileInfo(applicationDirPath() + QLatin1String("/../Resources/locale/")).absoluteFilePath();
#else
	m_localePath = OTTER_INSTALL_PREFIX + QLatin1String("/share/otter-browser/locale/");
#endif

	if (QFile::exists(applicationDirPath() + QLatin1String("/locale/")))
	{
		m_localePath = applicationDirPath() + QLatin1String("/locale/");
	}

	setLocale(QLatin1String("system"));

	m_commandLineParser.addHelpOption();
	m_commandLineParser.addVersionOption();
	m_commandLineParser.addPositionalArgument(QLatin1String("url"), translate("main", "URL to open"), QLatin1String("[url]"));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("cache"), translate("main", "Uses <path> as cache directory"), QLatin1String("path"), {}));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("profile"), translate("main", "Uses <path> as profile directory"), QLatin1String("path"), {}));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("session"), translate("main", "Restores session <session> if it exists"), QLatin1String("session"), {}));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("private-session"), translate("main", "Starts private session")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("session-chooser"), translate("main", "Forces session chooser dialog")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("portable"), translate("main", "Sets profile and cache paths to directories inside the same directory as that of application binary")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-tab"), translate("main", "Loads URL in new tab")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-private-tab"), translate("main", "Loads URL in new private tab")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-window"), translate("main", "Loads URL in new window")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("new-private-window"), translate("main", "Loads URL in new private window")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("readonly"), translate("main", "Tells application to avoid writing data to disk")));
	m_commandLineParser.addOption(QCommandLineOption(QLatin1String("report"), translate("main", "Prints out diagnostic report and exits application")));

	QStringList arguments(this->arguments());
	QString argumentsPath(QDir::current().filePath(QLatin1String("arguments.txt")));

	if (!QFile::exists(argumentsPath))
	{
		argumentsPath = QDir(applicationDirPath()).filePath(QLatin1String("arguments.txt"));
	}

	if (QFile::exists(argumentsPath))
	{
		QFile file(argumentsPath);

		if (file.open(QIODevice::ReadOnly))
		{
			QStringList temporaryArguments(QString(file.readAll()).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts));

			if (!temporaryArguments.isEmpty())
			{
				if (arguments.isEmpty())
				{
					temporaryArguments.prepend(QFileInfo(applicationFilePath()).fileName());
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

			file.close();
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

#ifdef Q_OS_WIN
		if (!rawReportOptions.contains(QLatin1String("noDialog")))
		{
			rawReportOptions.append(QLatin1String("dialog"));
		}
#endif

		if (rawReportOptions.isEmpty() || rawReportOptions.contains(QLatin1String("standard")))
		{
			reportOptions = StandardReport;
		}
		else if (rawReportOptions.contains(QLatin1String("full")))
		{
			reportOptions = FullReport;
		}
		else
		{
			if (rawReportOptions.contains(QLatin1String("environment")))
			{
				reportOptions |= EnvironmentReport;
			}

			if (rawReportOptions.contains(QLatin1String("keyboardShortcuts")))
			{
				reportOptions |= KeyboardShortcutsReport;
			}

			if (rawReportOptions.contains(QLatin1String("paths")))
			{
				reportOptions |= PathsReport;
			}

			if (rawReportOptions.contains(QLatin1String("settings")))
			{
				reportOptions |= SettingsReport;
			}
		}

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
		encodedArguments.reserve(decodedArguments.count());

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

	connect(m_localServer, &QLocalServer::newConnection, this, &Application::handleNewConnection);

	m_localServer->setSocketOptions(QLocalServer::UserAccessOption);

	if (!m_localServer->listen(server) && m_localServer->serverError() == QAbstractSocket::AddressInUseError && QLocalServer::removeServer(server))
	{
		m_localServer->listen(server);
	}

	Console::createInstance();

	SettingsManager::createInstance(profilePath);

	if (!isReadOnly && !QFileInfo(profilePath).isWritable())
	{
		QMessageBox::warning(nullptr, tr("Warning"), tr("Profile directory (%1) is not writable, application will be running in read-only mode.").arg(profilePath), QMessageBox::Close);

		isReadOnly = true;
	}

	if (!isReadOnly)
	{
		if (!QFile::exists(profilePath))
		{
			QDir().mkpath(profilePath);
		}

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

				const QAbstractButton *ignoreButton(messageBox.addButton(tr("Ignore"), QMessageBox::ActionRole));
				const QAbstractButton *quitButton(messageBox.addButton(tr("Quit"), QMessageBox::RejectRole));

				messageBox.exec();

				if (messageBox.clickedButton() == quitButton)
				{
					exit();

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

		if (m_localServer)
		{
			m_localServer->close();
		}

		exit();

		return;
	}

	ThemesManager::createInstance();

	ActionsManager::createInstance();

	AddonsManager::createInstance();

	BookmarksManager::createInstance();

	FeedsManager::createInstance();

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
	const int updateCheckInterval(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

	if (updateCheckInterval > 0 && (lastUpdate.isNull() ? updateCheckInterval : lastUpdate.daysTo(QDate::currentDate())) >= updateCheckInterval && !SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList().isEmpty())
	{
		connect(new UpdateChecker(this), &UpdateChecker::finished, this, &Application::handleUpdateCheckResult);

		if (!m_updateCheckTimer)
		{
			m_updateCheckTimer = new LongTermTimer(this);
			m_updateCheckTimer->start(static_cast<quint64>(updateCheckInterval * 1000 * SECONDS_IN_DAY));

			connect(m_updateCheckTimer, &LongTermTimer::timeout, this, &Application::periodicUpdateCheck);
		}
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

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &Application::handleOptionChanged);
	connect(this, &Application::aboutToQuit, this, &Application::handleAboutToQuit);
	connect(this, &Application::focusObjectChanged, this, &Application::handleFocusObjectChanged);
}

Application::~Application()
{
	for (int i = 0; i < m_windows.count(); ++i)
	{
		m_windows.at(i)->deleteLater();
	}
}

void Application::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	triggerAction(identifier, parameters, nullptr, trigger);
}

void Application::triggerAction(int identifier, const QVariantMap &parameters, QObject *target, ActionsManager::TriggerType trigger)
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
						triggerAction(actionIdentifier.toInt(), actionParameters, target, trigger);
					}
					else
					{
						triggerAction(ActionsManager::getActionIdentifier(actionIdentifier.toString()), actionParameters, target, trigger);
					}
				}
			}

			return;
		case ActionsManager::SetOptionAction:
			{
				const QString mode(parameters.value(QLatin1String("mode"), QLatin1String("set")).toString());
				const QString host(parameters.value(QLatin1String("host")).toString());
				const int option(SettingsManager::getOptionIdentifier(parameters.value(QLatin1String("option")).toString()));

				if (mode == QLatin1String("set"))
				{
					SettingsManager::setOption(option, parameters.value(QLatin1String("value")), host);
				}
				else if (mode == QLatin1String("reset"))
				{
					SettingsManager::setOption(option, {}, host);
				}
				else if (mode == QLatin1String("toggle"))
				{
					const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(option));

					switch (definition.type)
					{
						case SettingsManager::BooleanType:
							SettingsManager::setOption(option, !SettingsManager::getOption(option, host).toBool(), host);

							break;
						case SettingsManager::EnumerationType:
							if (definition.choices.count() > 1)
							{
								const QString value(SettingsManager::getOption(option, host).toString());

								if (value == definition.choices.last().value)
								{
									SettingsManager::setOption(option, definition.choices.first().value, host);
								}
								else
								{
									for (int i = 0; i < (definition.choices.count() - 1); ++i)
									{
										if (value == definition.choices.at(i).value)
										{
											SettingsManager::setOption(option, definition.choices.at(i + 1).value, host);

											break;
										}
									}
								}
							}

							break;
						default:
							break;
					}
				}
			}

			return;
		case ActionsManager::NewWindowAction:
		case ActionsManager::NewWindowPrivateAction:
			{
				SessionMainWindow session;

				if (m_activeWindow)
				{
					const SessionMainWindow activeSession(m_activeWindow->getSession());

					session.splitters = activeSession.splitters;
					session.hasToolBarsState = true;

					if (parameters.value(QLatin1String("minimalInterface")).toBool())
					{
						session.toolBars = {m_activeWindow->getToolBarState(ToolBarsManager::AddressBar), m_activeWindow->getToolBarState(ToolBarsManager::ProgressBar)};
					}
					else
					{
						session.toolBars = activeSession.toolBars;
					}
				}

				QVariantMap mutableParameters(parameters);

				if (identifier == ActionsManager::NewWindowPrivateAction)
				{
					mutableParameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
				}

				createWindow(mutableParameters, session);
			}

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
					if (m_windows.at(i)->isPrivate())
					{
						m_windows.at(i)->close();
					}
					else
					{
						m_windows.at(i)->triggerAction(ActionsManager::ClosePrivateTabsAction);
					}
				}
			}

			return;
		case ActionsManager::ReopenWindowAction:
			SessionsManager::restoreClosedWindow(parameters.value(QLatin1String("index"), 0).toInt());

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
		case ActionsManager::ActivateWindowAction:
			{
				const quint64 windowIdentifier(parameters.value(QLatin1String("window")).toULongLong());

				for (int i = 0; i < m_windows.count(); ++i)
				{
					if (m_windows.at(i)->getIdentifier() == windowIdentifier)
					{
						m_windows.at(i)->raise();
						m_windows.at(i)->activateWindow();

						break;
					}
				}
			}

			return;
		case ActionsManager::WorkOfflineAction:
			SettingsManager::setOption(SettingsManager::Network_WorkOfflineOption, parameters.value(QLatin1String("isChecked"), !m_instance->getActionState(identifier, parameters).isChecked).toBool());

			return;
		case ActionsManager::LockToolBarsAction:
			SettingsManager::setOption(SettingsManager::Interface_LockToolBarsOption, parameters.value(QLatin1String("isChecked"), !m_instance->getActionState(identifier, parameters).isChecked).toBool());

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
				ReportDialog *dialog(new ReportDialog(Application::StandardReport, m_activeWindow));
				dialog->setAttribute(Qt::WA_DeleteOnClose, true);
				dialog->show();
			}

			return;
		case ActionsManager::AboutApplicationAction:
			{
				const WebBackend *webBackend(AddonsManager::getWebBackend());
				QString about(tr("<b>Otter %1</b><br>Web browser controlled by the user, not vice-versa.<br><a href=\"https://www.otter-browser.org/\">https://www.otter-browser.org/</a>").arg(Application::getFullVersion()));

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

	if (scope == ActionsManager::ActionDefinition::MainWindowScope || scope == ActionsManager::ActionDefinition::WindowScope || scope == ActionsManager::ActionDefinition::EditorScope)
	{
		MainWindow *mainWindow(nullptr);

		if (parameters.contains(QLatin1String("window")))
		{
			const quint64 windowIdentifier(parameters.value(QLatin1String("window")).toULongLong());

			for (int i = 0; i < m_windows.count(); ++i)
			{
				if (m_windows.at(i)->getIdentifier() == windowIdentifier)
				{
					mainWindow = m_windows.at(i);

					break;
				}
			}
		}
		else
		{
			mainWindow = (target ? MainWindow::findMainWindow(target) : m_activeWindow.data());
		}

		if (scope == ActionsManager::ActionDefinition::WindowScope)
		{
			Window *window(nullptr);

			if (target)
			{
				if (mainWindow && parameters.contains(QLatin1String("tab")))
				{
					window = mainWindow->getWindowByIdentifier(parameters[QLatin1String("tab")].toULongLong());
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
				window->triggerAction(identifier, parameters, trigger);
			}
		}
		else if (mainWindow)
		{
			mainWindow->triggerAction(identifier, parameters, trigger);
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

void Application::removeWindow(MainWindow *mainWindow)
{
	m_windows.removeAll(mainWindow);

	mainWindow->deleteLater();

	emit m_instance->windowRemoved(mainWindow);

	if (m_windows.isEmpty())
	{
		exit();
	}
}

void Application::periodicUpdateCheck()
{
	UpdateChecker *updateChecker(new UpdateChecker(this));

	connect(updateChecker, &UpdateChecker::finished, this, &Application::handleUpdateCheckResult);

	const int interval(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

	if (!m_updateCheckTimer && interval > 0 && !SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList().isEmpty())
	{
		m_updateCheckTimer = new LongTermTimer(this);
		m_updateCheckTimer->start(static_cast<quint64>(interval * 1000 * SECONDS_IN_DAY));

		connect(m_updateCheckTimer, &LongTermTimer::timeout, this, &Application::periodicUpdateCheck);
	}
}

void Application::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Browser_EnableTrayIconOption:
			if (!m_trayIcon && value.toBool())
			{
				m_trayIcon = new TrayIcon(this);
			}
			else if (m_trayIcon && !value.toBool())
			{
				m_trayIcon->deleteLater();
				m_trayIcon = nullptr;
			}

			break;
		case SettingsManager::Browser_LocaleOption:
			setLocale(value.toString());

			break;
		case SettingsManager::Interface_LockToolBarsOption:
			emit arbitraryActionsStateChanged({ActionsManager::LockToolBarsAction});

			break;
		case SettingsManager::Network_WorkOfflineOption:
			emit arbitraryActionsStateChanged({ActionsManager::WorkOfflineAction});

			break;
		default:
			break;
	}
}

void Application::handleAboutToQuit()
{
	m_isAboutToQuit = true;

	if (m_localServer)
	{
		m_localServer->close();
	}

	QStringList clearSettings(SettingsManager::getOption(SettingsManager::History_ClearOnCloseOption).toStringList());
	clearSettings.removeAll({});

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

	QString data;
	QTextStream stream(socket);
	stream >> data;

	const QStringList encodedArguments(QString(QByteArray::fromBase64(data.toUtf8())).split(QLatin1Char(' ')));
	QStringList decodedArguments;
	decodedArguments.reserve(encodedArguments.count());

	for (int i = 0; i < encodedArguments.count(); ++i)
	{
		decodedArguments.append(QString(QByteArray::fromBase64(encodedArguments.at(i).toUtf8())));
	}

	m_commandLineParser.parse(decodedArguments);

	const QString session(m_commandLineParser.value(QLatin1String("session")));
	const bool isPrivate(m_commandLineParser.isSet(QLatin1String("private-session")));

	if (session.isEmpty())
	{
		const MainWindow *mainWindow(getWindows().isEmpty() ? nullptr : getWindow());

		if (!mainWindow || !SettingsManager::getOption(SettingsManager::Browser_OpenLinksInNewTabOption).toBool() || (isPrivate && !mainWindow->isPrivate()))
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

	handlePositionalArguments(&m_commandLineParser, true);

	delete socket;
}

void Application::handleFocusObjectChanged(QObject *object)
{
	if (!object || (object && !object->inherits("QMenu")))
	{
		m_nonMenuFocusObject = object;
	}
}

void Application::handlePositionalArguments(QCommandLineParser *parser, bool forceOpen)
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

	const QStringList urls((openHints == SessionsManager::DefaultOpen || !parser->positionalArguments().isEmpty()) ? parser->positionalArguments() : QStringList({}));

	if (!forceOpen && urls.isEmpty())
	{
		return;
	}

	MainWindow *mainWindow(nullptr);

	if (openHints.testFlag(SessionsManager::NewWindowOpen))
	{
		QVariantMap parameters;

		if (openHints.testFlag(SessionsManager::PrivateOpen))
		{
			parameters[QLatin1String("hints")] = SessionsManager::PrivateOpen;
		}

		if (urls.isEmpty())
		{
			mainWindow = createWindow(parameters);
		}
		else
		{
			for (int i = 0; i < urls.count(); ++i)
			{
				mainWindow = createWindow(parameters);

				if (!urls.at(i).isEmpty())
				{
					mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), urls.at(i)}, {QLatin1String("needsInterpretation"), true}});
				}
			}
		}
	}
	else
	{
		mainWindow = getWindow();

		if (mainWindow)
		{
			if (mainWindow->isSessionRestored())
			{
				if (urls.isEmpty())
				{
					mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("hints"), QVariant(openHints)}});
				}
				else
				{
					for (int i = 0; i < urls.count(); ++i)
					{
						mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), urls.at(i)}, {QLatin1String("needsInterpretation"), true}, {QLatin1String("hints"), QVariant(openHints)}});
					}
				}
			}
			else
			{
				connect(mainWindow, &MainWindow::sessionRestored, [=]()
				{
					if (urls.isEmpty())
					{
						mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("hints"), QVariant(openHints)}});
					}
					else
					{
						for (int i = 0; i < urls.count(); ++i)
						{
							mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), urls.at(i)}, {QLatin1String("needsInterpretation"), true}, {QLatin1String("hints"), QVariant(openHints)}});
						}
					}
				});
			}
		}
	}

	if (mainWindow)
	{
		mainWindow->raise();
		mainWindow->activateWindow();

		if (m_isHidden)
		{
			setHidden(false);
		}
		else
		{
			mainWindow->raiseWindow();
		}
	}
}

void Application::handleUpdateCheckResult(const QVector<UpdateChecker::UpdateInformation> &availableUpdates, int latestVersionIndex)
{
	if (availableUpdates.isEmpty())
	{
		return;
	}

	if (SettingsManager::getOption(SettingsManager::Updates_AutomaticInstallOption).toBool())
	{
		new Updater(availableUpdates.at(latestVersionIndex), this);
	}
	else
	{
		Notification *notification(NotificationsManager::createNotification(NotificationsManager::UpdateAvailableEvent, tr("New update %1 from %2 channel is available!").arg(availableUpdates.at(latestVersionIndex).version).arg(availableUpdates.at(latestVersionIndex).channel)));
		notification->setData(QVariant::fromValue<QVector<UpdateChecker::UpdateInformation> >(availableUpdates));

		connect(notification, &Notification::clicked, this, &Application::showUpdateDetails);
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
	const bool useSystemLocale(locale.isEmpty() || locale == QLatin1String("system"));

	m_qtTranslator->load(QLatin1String("qt_") + (useSystemLocale ? QLocale::system().name() : identifier), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	m_applicationTranslator->load((locale.endsWith(QLatin1String(".qm")) ? locale : QLatin1String("otter-browser_") + (useSystemLocale ? QLocale::system().name() : locale)), m_localePath);

	QLocale::setDefault(Utils::createLocale(identifier));
}

MainWindow* Application::createWindow(const QVariantMap &parameters, const SessionMainWindow &session)
{
	MainWindow *mainWindow(new MainWindow(parameters, session));

	m_windows.prepend(mainWindow);

	const bool inBackground(parameters.contains(QLatin1String("hints")) ? SessionsManager::calculateOpenHints(parameters).testFlag(SessionsManager::BackgroundOpen) : false);

	if (inBackground)
	{
		mainWindow->setAttribute(Qt::WA_ShowWithoutActivating, true);
	}
	else
	{
		m_activeWindow = mainWindow;
	}

	mainWindow->show();

	if (inBackground)
	{
		mainWindow->lower();
		mainWindow->setAttribute(Qt::WA_ShowWithoutActivating, false);
	}

	if (parameters.contains(QLatin1String("url")))
	{
		mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), parameters[QLatin1String("url")].toString()}, {QLatin1String("needsInterpretation"), true}});
	}

	emit m_instance->windowAdded(mainWindow);

	connect(mainWindow, &MainWindow::activated, [=]()
	{
		m_activeWindow = mainWindow;

		emit m_instance->currentWindowChanged(mainWindow);
	});

	return mainWindow;
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

QObject* Application::getFocusObject(bool ignoreMenus)
{
	return (ignoreMenus ? m_nonMenuFocusObject.data() : focusObject());
}

Style* Application::getStyle()
{
	Style *style(qobject_cast<Style*>(QApplication::style()));

	if (style)
	{
		return style;
	}

	return QApplication::style()->findChild<Style*>();
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

ActionsManager::ActionDefinition::State Application::getActionState(int identifier, const QVariantMap &parameters) const
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(identifier));

	switch (definition.scope)
	{
		case ActionsManager::ActionDefinition::EditorScope:
		case ActionsManager::ActionDefinition::WindowScope:
		case ActionsManager::ActionDefinition::MainWindowScope:
			if (m_activeWindow)
			{
				return m_activeWindow->getActionState(identifier, parameters);
			}

			return definition.getDefaultState();
		case ActionsManager::ActionDefinition::ApplicationScope:
			break;
		default:
			return definition.getDefaultState();
	}

	ActionsManager::ActionDefinition::State state(definition.getDefaultState());

	switch (identifier)
	{
		case ActionsManager::RunMacroAction:
			state.isEnabled = parameters.contains(QLatin1String("actions"));

			break;
		case ActionsManager::SetOptionAction:
			if (!parameters.isEmpty())
			{
				const QString mode(parameters.value(QLatin1String("mode"), QLatin1String("set")).toString());
				const QString host(parameters.value(QLatin1String("host")).toString());
				const QString name(parameters.value(QLatin1String("option")).toString());

				state.isEnabled = true;

				if (mode == QLatin1String("set"))
				{
					if (host.isEmpty())
					{
						state.text = translate("actions", "Set %1").arg(name);
					}
					else
					{
						state.text = translate("actions", "Set %1 for %2").arg(name).arg(host);
					}
				}
				else if (mode == QLatin1String("reset"))
				{
					if (host.isEmpty())
					{
						state.text = translate("actions", "Reset %1").arg(name);
					}
					else
					{
						state.text = translate("actions", "Reset %1 for %2").arg(name).arg(host);
					}
				}
				else if (mode == QLatin1String("toggle"))
				{
					if (SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(name)).type == SettingsManager::BooleanType)
					{
						if (host.isEmpty())
						{
							state.text = translate("actions", "Toggle %1").arg(name);
						}
						else
						{
							state.text = translate("actions", "Toggle %1 for %2").arg(name).arg(host);
						}
					}
					else
					{
						state.isEnabled = false;
					}
				}
			}

			break;
		case ActionsManager::ReopenWindowAction:
			if (!SessionsManager::getClosedWindows().isEmpty())
			{
				if (parameters.contains(QLatin1String("index")))
				{
					const int index(parameters[QLatin1String("index")].toInt());

					state.isEnabled = (index >= 0 && index < SessionsManager::getClosedWindows().count());
				}
				else
				{
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::ActivateWindowAction:
			{
				const quint64 windowIdentifier(parameters.value(QLatin1String("window")).toULongLong());

				for (int i = 0; i < m_windows.count(); ++i)
				{
					if (m_windows.at(i)->getIdentifier() == windowIdentifier)
					{
						state.isEnabled = true;

						break;
					}
				}
			}

			break;
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
	if (TransfersManager::hasRunningTransfers() && SettingsManager::getOption(SettingsManager::Choices_WarnQuitTransfersOption).toBool())
	{
		const QVector<Transfer*> transfers(TransfersManager::getTransfers());
		int runningTransfers(0);

		for (int i = 0; i < transfers.count(); ++i)
		{
			if (transfers.at(i)->getState() == Transfer::RunningState)
			{
				++runningTransfers;
			}
		}

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

		if (result == QMessageBox::Yes)
		{
			return true;
		}

		if (messageBox.clickedButton() == hideButton)
		{
			setHidden(true);
		}

		return false;
	}

	const QString warnQuitMode(SettingsManager::getOption(SettingsManager::Choices_WarnQuitOption).toString());

	if (warnQuitMode == QLatin1String("noWarn"))
	{
		return true;
	}

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

		if (result == QMessageBox::Yes)
		{
			return true;
		}

		if (messageBox.clickedButton() == hideButton)
		{
			setHidden(true);
		}

		return false;
	}

	return true;
}

bool Application::isAboutToQuit()
{
	return (m_isAboutToQuit || closingDown());
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
