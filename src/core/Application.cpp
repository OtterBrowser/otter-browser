/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "TasksManager.h"
#include "ToolBarsManager.h"
#include "ThemesManager.h"
#include "TransfersManager.h"
#include "Utils.h"
#include "Updater.h"
#include "WebBackend.h"
#ifdef Q_OS_WIN
#include "../modules/platforms/windows/WindowsPlatformIntegration.h"
#elif defined(Q_OS_DARWIN)
#include "../modules/platforms/mac/MacPlatformIntegration.h"
#elif defined(Q_OS_UNIX)
#include "../modules/platforms/freedesktoporg/FreeDesktopOrgPlatformIntegration.h"
#endif
#include "../ui/DataExchangerDialog.h"
#include "../ui/DiagnosticReportDialog.h"
#include "../ui/LocaleDialog.h"
#include "../ui/MainWindow.h"
#include "../ui/NotificationDialog.h"
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
#if QT_VERSION < 0x060000
#include <QtCore/QTextCodec>
#endif
#include <QtCore/QTranslator>
#include <QtGui/QDesktopServices>
#include <QtNetwork/QLocalSocket>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#define MESSAGE_IDENTIFIER ""
#define MESSAGE_URL ""

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
bool Application::m_isFirstRun(false);
bool Application::m_isHidden(false);
bool Application::m_isUpdating(false);

Application::Application(int &argc, char **argv) : QApplication(argc, argv),
	m_updateCheckTimer(nullptr)
{
	setApplicationName(QLatin1String("Otter"));
	setApplicationDisplayName(QLatin1String("Otter Browser"));
	setApplicationVersion(OTTER_VERSION_MAIN);
	setWindowIcon(QIcon::fromTheme(QLatin1String("otter-browser"), QIcon(QLatin1String(":/icons/otter-browser.png"))));

	m_instance = this;

#if QT_VERSION < 0x060000
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));
#endif

	QString profilePath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/otter"));
	QString cachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

#ifdef Q_OS_DARWIN
	m_localePath = QFileInfo(applicationDirPath() + QLatin1String("/../Resources/locale/")).absoluteFilePath();
#else
	m_localePath = QFileInfo(applicationDirPath() + QLatin1String("/../share/otter-browser/locale/")).absoluteFilePath();
#endif

	const QString localLocalePath(applicationDirPath() + QLatin1String("/locale/"));

	if (QFile::exists(localLocalePath))
	{
		m_localePath = localLocalePath;
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

	QStringList arguments(Application::arguments());
	QString argumentsPath(QDir::current().filePath(QLatin1String("arguments.txt")));
	const QString applicationDirectoryPath(getApplicationDirectoryPath());

	if (!QFile::exists(argumentsPath))
	{
		argumentsPath = QDir(applicationDirectoryPath).filePath(QLatin1String("arguments.txt"));
	}

	if (QFile::exists(argumentsPath))
	{
		QFile file(argumentsPath);

		if (file.open(QIODevice::ReadOnly))
		{
			QStringList temporaryArguments(QString::fromLatin1(file.readAll()).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts));

			if (!temporaryArguments.isEmpty())
			{
				if (arguments.isEmpty())
				{
					temporaryArguments.prepend(QFileInfo(applicationFilePath()).fileName());
				}
				else
				{
					temporaryArguments.prepend(arguments.value(0));
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

	const bool isPrivate(m_commandLineParser.isSet(QLatin1String("private-session")));
	bool isReadOnly(m_commandLineParser.isSet(QLatin1String("readonly")));

	if (m_commandLineParser.isSet(QLatin1String("portable")))
	{
		profilePath = applicationDirectoryPath + QLatin1String("/profile");
		cachePath = applicationDirectoryPath + QLatin1String("/cache");
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
			DiagnosticReportDialog dialog(reportOptions);
			dialog.exec();
		}
		else
		{
			QTextStream stream(stdout);
			stream << createReport(reportOptions);
			stream.flush();
		}

		return;
	}

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(profilePath.toUtf8());

	const QString serverName(applicationName() + QLatin1Char('-') + QString::fromLatin1(hash.result().toHex()));
	QLocalSocket socket;
	socket.connectToServer(serverName);

	if (socket.waitForConnected(500))
	{
		QStringList encodedArguments;
		encodedArguments.reserve(arguments.count());

#ifdef Q_OS_WIN
		AllowSetForegroundWindow(ASFW_ANY);
#endif

		for (int i = 0; i < arguments.count(); ++i)
		{
			encodedArguments.append(QString::fromLatin1(arguments.at(i).toUtf8().toBase64()));
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

	if (!m_localServer->listen(serverName) && m_localServer->serverError() == QAbstractSocket::AddressInUseError && QLocalServer::removeServer(serverName))
	{
		m_localServer->listen(serverName);
	}

	m_isFirstRun = !QFile::exists(profilePath);

	Console::createInstance();

	SettingsManager::createInstance(profilePath);

	if (!isReadOnly && !m_isFirstRun && !QFileInfo(profilePath).isWritable())
	{
		QMessageBox::warning(nullptr, tr("Warning"), tr("Profile directory (%1) is not writable, application will be running in read-only mode.").arg(profilePath), QMessageBox::Close);

		isReadOnly = true;
	}

	if (!isReadOnly)
	{
		if (m_isFirstRun)
		{
			Utils::ensureDirectoryExists(profilePath);
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
					message = tr("Your profile directory (%1) is running low on free disk space (%2 remaining).\nThis may lead to malfunctions or even data loss.").arg(QDir::toNativeSeparators(profilePath), Utils::formatUnit(storageInformation.bytesAvailable()));
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

	TasksManager::createInstance();

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

	if (!QSslSocket::supportsSsl() || (webBackend && !webBackend->hasSslSupport()))
	{
		QMessageBox::warning(nullptr, tr("Warning"), tr("SSL support is not available or is incomplete.\nSome websites may work incorrectly or do not work at all."), QMessageBox::Close);
	}

#ifdef Q_OS_WIN
	m_platformIntegration = new WindowsPlatformIntegration(this);
#elif defined(Q_OS_DARWIN)
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

		scheduleUpdateCheck(updateCheckInterval);
	}

	Style *style(ThemesManager::createStyle(SettingsManager::getOption(SettingsManager::Interface_WidgetStyleOption).toString()));
	QString styleSheet(style->getStyleSheet());
	const QString styleSheetPath(SettingsManager::getOption(SettingsManager::Interface_StyleSheetOption).toString());

	if (!styleSheetPath.isEmpty())
	{
		QFile file(styleSheetPath);

		if (file.open(QIODevice::ReadOnly))
		{
			styleSheet += QString::fromLatin1(file.readAll());

			file.close();
		}
	}

	handleOptionChanged(SettingsManager::Browser_EnableTrayIconOption, SettingsManager::getOption(SettingsManager::Browser_EnableTrayIconOption));
	setStyle(style);
	setStyleSheet(styleSheet);

	QDesktopServices::setUrlHandler(QLatin1String("feed"), this, "openUrl");
	QDesktopServices::setUrlHandler(QLatin1String("ftp"), this, "openUrl");
	QDesktopServices::setUrlHandler(QLatin1String("http"), this, "openUrl");
	QDesktopServices::setUrlHandler(QLatin1String("https"), this, "openUrl");

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &Application::handleOptionChanged);
	connect(this, &Application::aboutToQuit, this, &Application::handleAboutToQuit);
	connect(this, &Application::focusObjectChanged, this, [&](QObject *object)
	{
		if (!object || !object->inherits("QMenu"))
		{
			m_nonMenuFocusObject = object;
		}
	});
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
					const QVariant action(actions.at(i));
					QVariant actionIdentifier;
					QVariantMap actionParameters;

					if (action.type() == QVariant::Map)
					{
						const QVariantMap actionData(action.toMap());

						actionIdentifier = actionData.value(QLatin1String("identifier"));
						actionParameters = actionData.value(QLatin1String("parameters")).toMap();
					}
					else
					{
						actionIdentifier = action;
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
									SettingsManager::setOption(option, definition.choices.value(0).value, host);
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
				Session::MainWindow session;

				if (m_activeWindow)
				{
					const Session::MainWindow activeSession(m_activeWindow->getSession());

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
				triggerAction(ActionsManager::ExitAction, {}, m_instance);
			}
			else
			{
				for (int i = 0; i < m_windows.count(); ++i)
				{
					MainWindow *window(m_windows.at(i));

					if (window->isPrivate())
					{
						window->close();
					}
					else
					{
						window->triggerAction(ActionsManager::ClosePrivateTabsAction);
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
		case ActionsManager::AddSearchAction:
			if (parameters.contains(QLatin1String("url")))
			{
				SearchEngineFetchJob *job(new SearchEngineFetchJob(parameters[QLatin1String("url")].toUrl(), {}, true, m_instance));

				connect(job, &SearchEngineFetchJob::jobFinished, m_instance, [&](bool isSuccess)
				{
					if (!isSuccess)
					{
						QMessageBox::warning(m_activeWindow, tr("Error"), tr("Failed to add search engine."), QMessageBox::Close);
					}
				});

				job->start();
			}

			break;
		case ActionsManager::ActivateWindowAction:
			if (parameters.contains(QLatin1String("window")))
			{
				const quint64 windowIdentifier(parameters.value(QLatin1String("window")).toULongLong());

				for (int i = 0; i < m_windows.count(); ++i)
				{
					MainWindow *window(m_windows.at(i));

					if (window->getIdentifier() == windowIdentifier)
					{
						window->raise();
						window->activateWindow();

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
		case ActionsManager::ExchangeDataAction:
			if (parameters.contains(QLatin1String("exchanger")))
			{
				DataExchangerDialog::createDialog(parameters.value(QLatin1String("exchanger")).toString(), m_activeWindow);
			}

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
				DiagnosticReportDialog *dialog(new DiagnosticReportDialog(Application::StandardReport, m_activeWindow));
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

					about.append(QLatin1String("<br><br>") + tr("Web backend: %1 %2.").arg(webBackend->getTitle(), webBackend->getEngineVersion()) + QLatin1String("<br><br>"));

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
			if (canClose())
			{
				SessionsManager::saveSession();

				exit();
			}

			return;
		default:
			break;
	}

	const ActionsManager::ActionDefinition::ActionScope scope(ActionsManager::getActionDefinition(identifier).scope);

	if (scope != ActionsManager::ActionDefinition::MainWindowScope && scope != ActionsManager::ActionDefinition::WindowScope && scope != ActionsManager::ActionDefinition::EditorScope)
	{
		return;
	}

	MainWindow *mainWindow(nullptr);

	if (parameters.contains(QLatin1String("window")))
	{
		const quint64 windowIdentifier(parameters.value(QLatin1String("window")).toULongLong());

		for (int i = 0; i < m_windows.count(); ++i)
		{
			MainWindow *currentMainWindow(m_windows.at(i));

			if (currentMainWindow->getIdentifier() == windowIdentifier)
			{
				mainWindow = currentMainWindow;

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

void Application::scheduleUpdateCheck(quint64 interval)
{
	if (m_updateCheckTimer)
	{
		return;
	}

	m_updateCheckTimer = new LongTermTimer(this);
	m_updateCheckTimer->start(interval * 1000 * 86400);

	connect(m_updateCheckTimer, &LongTermTimer::timeout, this, [&]()
	{
		UpdateChecker *updateChecker(new UpdateChecker(this));

		connect(updateChecker, &UpdateChecker::finished, this, &Application::handleUpdateCheckResult);

		const int interval(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

		if (interval > 0 && !SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList().isEmpty())
		{
			scheduleUpdateCheck(interval);
		}
	});
}

void Application::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Backends_WebOption:
			emit arbitraryActionsStateChanged({ActionsManager::ExchangeDataAction});

			break;
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

	if (clearSettings.isEmpty())
	{
		return;
	}

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

	const QStringList encodedArguments(QString::fromLatin1(QByteArray::fromBase64(data.toUtf8())).split(QLatin1Char(' ')));
	QStringList decodedArguments;
	decodedArguments.reserve(encodedArguments.count());

	for (int i = 0; i < encodedArguments.count(); ++i)
	{
		decodedArguments.append(QString::fromLatin1(QByteArray::fromBase64(encodedArguments.at(i).toUtf8())));
	}

	m_commandLineParser.parse(decodedArguments);

	const QString session(m_commandLineParser.value(QLatin1String("session")));
	const bool isPrivate(m_commandLineParser.isSet(QLatin1String("private-session")));

	if (session.isEmpty())
	{
		const MainWindow *mainWindow(m_windows.value(0));

		if (!mainWindow || !SettingsManager::getOption(SettingsManager::Browser_OpenLinksInNewTabOption).toBool() || (isPrivate && !mainWindow->isPrivate()))
		{
			createWindow({{QLatin1String("hints"), (isPrivate ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}});
		}
	}
	else
	{
		const SessionInformation sessionData(SessionsManager::getSession(session));

		if (sessionData.isClean || QMessageBox::warning(nullptr, tr("Warning"), tr("This session was not saved correctly.\nAre you sure that you want to restore this session anyway?"), (QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes)
		{
			const QVariantMap parameters({{QLatin1String("hints"), (isPrivate ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}});

			for (int i = 0; i < sessionData.windows.count(); ++i)
			{
				createWindow(parameters, sessionData.windows.at(i));
			}
		}
	}

	handlePositionalArguments(&m_commandLineParser, true);

	delete socket;
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

	const QStringList urls((openHints == SessionsManager::DefaultOpen || !parser->positionalArguments().isEmpty()) ? parser->positionalArguments() : QStringList());

	if (!forceOpen && urls.isEmpty())
	{
		return;
	}

	MainWindow *mainWindow(nullptr);

	if (openHints.testFlag(SessionsManager::NewWindowOpen))
	{
		const QVariantMap parameters({{QLatin1String("hints"), (openHints.testFlag(SessionsManager::PrivateOpen) ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}});

		if (urls.isEmpty())
		{
			mainWindow = createWindow(parameters);
		}
		else
		{
			for (int i = 0; i < urls.count(); ++i)
			{
				const QUrl url(urls.at(i));

				mainWindow = createWindow(parameters);

				if (!url.isEmpty())
				{
					mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}, {QLatin1String("needsInterpretation"), true}});
				}
			}
		}
	}
	else
	{
		mainWindow = (m_windows.isEmpty() ? createWindow() : m_windows.value(0));

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

	const UpdateChecker::UpdateInformation availableUpdate(availableUpdates.at(latestVersionIndex));

	if (SettingsManager::getOption(SettingsManager::Updates_AutomaticInstallOption).toBool())
	{
		new Updater(availableUpdate, this);

		return;
	}

	Notification::Message message;
	message.message = tr("New update %1 from %2 channel is available.").arg(availableUpdate.version, availableUpdate.channel);
	message.icon = windowIcon();
	message.event = NotificationsManager::UpdateAvailableEvent;

	Notification *notification(NotificationsManager::createNotification(message));

	connect(notification, &Notification::clicked, notification, [=]()
	{
		UpdateCheckerDialog *dialog(new UpdateCheckerDialog(nullptr, availableUpdates));
		dialog->show();
	});
}

void Application::showNotification(Notification *notification)
{
	if (m_platformIntegration && m_platformIntegration->canShowNotifications() && SettingsManager::getOption(SettingsManager::Interface_UseNativeNotificationsOption).toBool())
	{
		m_platformIntegration->showNotification(notification);
	}
	else
	{
		NotificationDialog *dialog(new NotificationDialog(notification));
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
		MainWindow *mainWindow(m_windows.at(i));

		if (m_isHidden)
		{
			mainWindow->storeWindowState();
			mainWindow->hide();
		}
		else
		{
			mainWindow->show();
			mainWindow->activateWindow();
			mainWindow->restoreWindowState();
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

	const QString systemLocale(QLocale::system().name());
	QString identifier(locale);

	if (locale.endsWith(QLatin1String(".qm")))
	{
		identifier = QFileInfo(locale).baseName().remove(QLatin1String("otter-browser_"));
	}
	else if (locale == QLatin1String("system"))
	{
		identifier = systemLocale;
	}

	const bool useSystemLocale(locale.isEmpty() || locale == QLatin1String("system"));

	m_qtTranslator->load(QLatin1String("qt_") + (useSystemLocale ? systemLocale : identifier), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	m_applicationTranslator->load((locale.endsWith(QLatin1String(".qm")) ? locale : QLatin1String("otter-browser_") + (useSystemLocale ? systemLocale : locale)), m_localePath);

	QLocale::setDefault(Utils::createLocale(identifier));
}

MainWindow* Application::createWindow(const QVariantMap &parameters, const Session::MainWindow &session)
{
	MainWindow *mainWindow(new MainWindow(parameters, session));
	const QString messageIdentifier(MESSAGE_IDENTIFIER);

	if (m_windows.isEmpty() && !messageIdentifier.isEmpty())
	{
		QStringList identifiers(SettingsManager::getOption(SettingsManager::Browser_MessagesOption).toStringList());

		if (!identifiers.contains(messageIdentifier))
		{
			mainWindow->triggerAction(ActionsManager::NewTabAction, {{QLatin1String("url"), QString(MESSAGE_URL)}});

			identifiers.append(messageIdentifier);

			SettingsManager::setOption(SettingsManager::Browser_MessagesOption, identifiers);
		}
	}

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

	connect(mainWindow, &MainWindow::activated, mainWindow, [=]()
	{
		m_activeWindow = mainWindow;

		emit m_instance->activeWindowChanged(mainWindow);
	});

	return mainWindow;
}

Application* Application::getInstance()
{
	return m_instance;
}

MainWindow* Application::getActiveWindow()
{
	return m_activeWindow;
}

QObject* Application::getFocusObject(bool ignoreMenus)
{
	return (ignoreMenus ? m_nonMenuFocusObject.data() : focusObject());
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
	const QString versionWeekly(QStringLiteral(OTTER_VERSION_WEEKLY).trimmed());
	const QString gitBranch(QStringLiteral(OTTER_GIT_BRANCH).trimmed());
	const QString gitRevision(QStringLiteral(OTTER_GIT_REVISION).trimmed());
	DiagnosticReport report;
	DiagnosticReport::Section versionReport;
	versionReport.title = QLatin1String("Version");
	versionReport.fieldWidths = {20, 0};
	versionReport.entries.reserve(7);
	versionReport.entries.append({QLatin1String("Main Number"), OTTER_VERSION_MAIN});
	versionReport.entries.append({QLatin1String("Weekly Number"), (versionWeekly.isEmpty() ? QString(QLatin1Char('-')) : versionWeekly)});
	versionReport.entries.append({QLatin1String("Context"), OTTER_VERSION_CONTEXT});
	versionReport.entries.append({QLatin1String("Web Backend"), (webBackend ? QStringLiteral("%1 %2").arg(webBackend->getTitle(), webBackend->getEngineVersion()) : QLatin1String("none"))});
	versionReport.entries.append({QLatin1String("Build Date"), QDateTime::fromString(OTTER_BUILD_DATETIME, Qt::ISODate).toUTC().toString(Qt::ISODate)});
	versionReport.entries.append({QLatin1String("Git Branch"), ((gitBranch.isEmpty() || gitBranch == QLatin1String("unknown")) ? QString(QLatin1Char('-')) : gitBranch)});
	versionReport.entries.append({QLatin1String("Git Revision"), ((gitRevision.isEmpty() || gitRevision == QLatin1String("unknown")) ? QString(QLatin1Char('-')) : QStringLiteral("%1 (%2)").arg(gitRevision, QDateTime::fromString(QStringLiteral(OTTER_GIT_DATETIME).trimmed(), Qt::ISODate).toUTC().toString(Qt::ISODate)))});

	report.sections.append(versionReport);

	if (options.testFlag(EnvironmentReport))
	{
		const QString sslVersion(webBackend ? webBackend->getSslVersion() : QString());
		DiagnosticReport::Section environmentReport;
		environmentReport.title = QLatin1String("Environment");
		environmentReport.fieldWidths = {30, 0};
		environmentReport.entries.reserve(8);
		environmentReport.entries.append({QLatin1String("System Name"), QSysInfo::prettyProductName()});
		environmentReport.entries.append({QLatin1String("Build ABI"), QSysInfo::buildAbi()});
		environmentReport.entries.append({QLatin1String("Build CPU Architecture"), QSysInfo::buildCpuArchitecture()});
		environmentReport.entries.append({QLatin1String("Runtime CPU Architecture"), QSysInfo::currentCpuArchitecture()});
		environmentReport.entries.append({QLatin1String("Build Qt Version"), QT_VERSION_STR});
		environmentReport.entries.append({QLatin1String("Runtime Qt Version"), qVersion()});
		environmentReport.entries.append({QLatin1String("SSL Library Version"), (sslVersion.isEmpty() ? QLatin1String("unavailable") : sslVersion)});
		environmentReport.entries.append({QLatin1String("System Locale"), QLocale::system().name()});

		report.sections.append(environmentReport);
	}

	if (options.testFlag(PathsReport))
	{
		DiagnosticReport::Section pathsReport;
		pathsReport.title = QLatin1String("Paths");
		pathsReport.fieldWidths = {20, 0};
		pathsReport.entries.reserve(8);
		pathsReport.entries.append({QLatin1String("Profile"), SessionsManager::getProfilePath()});
		pathsReport.entries.append({QLatin1String("Configuration"), SettingsManager::getGlobalPath()});
		pathsReport.entries.append({QLatin1String("Overrides"), SettingsManager::getOverridePath()});
		pathsReport.entries.append({QLatin1String("Session"), SessionsManager::getSessionPath(SessionsManager::getCurrentSession())});
		pathsReport.entries.append({QLatin1String("Bookmarks"), SessionsManager::getWritableDataPath(QLatin1String("bookmarks.xbel"))});
		pathsReport.entries.append({QLatin1String("Notes"), SessionsManager::getWritableDataPath(QLatin1String("notes.xbel"))});
		pathsReport.entries.append({QLatin1String("History"), SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.json"))});
		pathsReport.entries.append({QLatin1String("Cache"), SessionsManager::getCachePath()});

		report.sections.append(pathsReport);
	}

	if (options.testFlag(SettingsReport))
	{
		report.sections.append(SettingsManager::createReport());
	}

	if (options.testFlag(KeyboardShortcutsReport))
	{
		report.sections.append(ActionsManager::createReport());
	}

	QString reportString;
	QTextStream stream(&reportString);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Otter Browser diagnostic report created on ") << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << QLatin1String("\n\n");

	for (int i = 0; i < report.sections.count(); ++i)
	{
		const DiagnosticReport::Section section(report.sections.at(i));

		stream << section.title << QLatin1String(":\n");

		for (int j = 0; j < section.entries.count(); ++j)
		{
			const QStringList fields(section.entries.at(j));
			int size(0);

			stream << QLatin1Char('\t');

			for (int k = 0; k < fields.count(); ++k)
			{
				size = section.fieldWidths.value(k, size);

				stream.setFieldWidth(size);
				stream << fields.at(k);
			}

			stream.setFieldWidth(0);
			stream << QLatin1Char('\n');
		}

		stream << QLatin1Char('\n');
	}

	return reportString.remove(QRegularExpression(QLatin1String(" +$"), QRegularExpression::MultilineOption));
}

QString Application::getFullVersion()
{
	return QStringLiteral("%1%2").arg(OTTER_VERSION_MAIN, OTTER_VERSION_CONTEXT);
}

QString Application::getLocalePath()
{
	return m_localePath;
}

QString Application::getApplicationDirectoryPath()
{
#if defined(Q_OS_DARWIN)
	return QDir::cleanPath(applicationDirPath() + QLatin1String("/../../../"));
#else
	return applicationDirPath();
#endif
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
						state.text = translate("actions", "Set %1 for %2").arg(name, host);
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
						state.text = translate("actions", "Reset %1 for %2").arg(name, host);
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
							state.text = translate("actions", "Toggle %1 for %2").arg(name, host);
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
			if (parameters.contains(QLatin1String("window")))
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
		case ActionsManager::ExchangeDataAction:
			if (parameters.contains(QLatin1String("exchanger")))
			{
				const QString exchanger(parameters.value(QLatin1String("exchanger")).toString());

				state.isEnabled = (!SessionsManager::isReadOnly() || exchanger.endsWith(QLatin1String("Export")));

				if (exchanger == QLatin1String("HtmlBookmarksImport"))
				{
					const WebBackend *webBackend(AddonsManager::getWebBackend());

					state.text = translate("actions", "Import HTML Bookmarks…");

					if (!webBackend || !webBackend->hasCapability(WebBackend::BookmarksImportCapability))
					{
						state.isEnabled = false;
					}
				}
				else if (exchanger == QLatin1String("OperaBookmarksImport"))
				{
					state.text = translate("actions", "Import Opera Bookmarks…");
				}
				else if (exchanger == QLatin1String("OperaNotesImport"))
				{
					state.text = translate("actions", "Import Opera Notes…");
				}
				else if (exchanger == QLatin1String("OperaSearchEnginesImport"))
				{
					state.text = translate("actions", "Import Opera Search Engines…");
				}
				else if (exchanger == QLatin1String("OperaSessionImport"))
				{
					state.text = translate("actions", "Import Opera Session…");
				}
				else if (exchanger == QLatin1String("OpmlFeedsImport"))
				{
					state.text = translate("actions", "Import OPML Feeds…");
				}
				else if (exchanger == QLatin1String("HtmlBookmarksExport"))
				{
					state.text = translate("actions", "Export Bookmarks as HTML…");
				}
				else if (exchanger == QLatin1String("XbelBookmarksExport"))
				{
					state.text = translate("actions", "Export Bookmarks as XBEL…");
				}
				else
				{
					state.isEnabled = false;
				}
			}

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
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You are about to quit while %n files are still being downloaded.", "", TransfersManager::getRunningTransfersCount()));
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
		tabsAmount += m_windows.at(i)->getWindowCount();
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

bool Application::isFirstRun()
{
	return m_isFirstRun;
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
