/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#include "Migrator.h"
#include "IniSettings.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "ToolBarsManager.h"
#include "../ui/ItemViewWidget.h"

#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

class OptionsRenameMigration final : public Migration
{
public:
	OptionsRenameMigration() : Migration()
	{
	}

	void createBackup() const override
	{
		const QString backupPath(createBackupPath(QLatin1String("other")));
		const QList<QFileInfo> entries(QDir(SessionsManager::getWritableDataPath(QString())).entryInfoList(QStringList({QLatin1String("otter.conf"), QLatin1String("override.ini")})));

		for (int i = 0; i < entries.count(); ++i)
		{
			QFile::copy(entries.at(i).absoluteFilePath(), backupPath + entries.at(i).fileName());
		}
	}

	void migrate() const override
	{
		QMap<QString, SettingsManager::OptionIdentifier> optionsMap;
		optionsMap[QLatin1String("Browser/EnableFullScreen")] = SettingsManager::Permissions_EnableFullScreenOption;
		optionsMap[QLatin1String("Browser/EnableGeolocation")] = SettingsManager::Permissions_EnableGeolocationOption;
		optionsMap[QLatin1String("Browser/EnableImages")] = SettingsManager::Permissions_EnableImagesOption;
		optionsMap[QLatin1String("Browser/EnableJavaScript")] = SettingsManager::Permissions_EnableJavaScriptOption;
		optionsMap[QLatin1String("Browser/EnableLocalStorage")] = SettingsManager::Permissions_EnableLocalStorageOption;
		optionsMap[QLatin1String("Browser/EnableMediaCaptureAudio")] = SettingsManager::Permissions_EnableMediaCaptureAudioOption;
		optionsMap[QLatin1String("Browser/EnableMediaCaptureVideo")] = SettingsManager::Permissions_EnableMediaCaptureVideoOption;
		optionsMap[QLatin1String("Browser/EnableMediaPlaybackAudio")] = SettingsManager::Permissions_EnableMediaPlaybackAudioOption;
		optionsMap[QLatin1String("Browser/EnableNotifications")] = SettingsManager::Permissions_EnableNotificationsOption;
		optionsMap[QLatin1String("Browser/EnableOfflineStorageDatabase")] = SettingsManager::Permissions_EnableOfflineStorageDatabaseOption;
		optionsMap[QLatin1String("Browser/EnableOfflineWebApplicationCache")] = SettingsManager::Permissions_EnableOfflineWebApplicationCacheOption;
		optionsMap[QLatin1String("Browser/EnablePlugins")] = SettingsManager::Permissions_EnablePluginsOption;
		optionsMap[QLatin1String("Browser/EnablePointerLock")] = SettingsManager::Permissions_EnablePointerLockOption;
		optionsMap[QLatin1String("Browser/EnableWebgl")] = SettingsManager::Permissions_EnableWebglOption;
		optionsMap[QLatin1String("Browser/JavaScriptCanAccessClipboard")] = SettingsManager::Permissions_ScriptsCanAccessClipboardOption;
		optionsMap[QLatin1String("Browser/JavaScriptCanChangeWindowGeometry")] = SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption;
		optionsMap[QLatin1String("Browser/JavaScriptCanCloseWindows")] = SettingsManager::Permissions_ScriptsCanCloseWindowsOption;
		optionsMap[QLatin1String("Browser/JavaScriptCanDisableContextMenu")] = SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption;
		optionsMap[QLatin1String("Browser/JavaScriptCanShowStatusMessages")] = SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption;
		optionsMap[QLatin1String("Browser/TabCrashingActionOption")] = SettingsManager::Interface_TabCrashingActionOption;
		optionsMap[QLatin1String("Content/PopupsPolicy")] = SettingsManager::Permissions_ScriptsCanOpenWindowsOption;

		QMap<QString, SettingsManager::OptionIdentifier>::iterator optionsIterator;
		QSettings configuration(SettingsManager::getGlobalPath(), QSettings::IniFormat);
		const QStringList configurationKeys(configuration.allKeys());

		for (optionsIterator = optionsMap.begin(); optionsIterator != optionsMap.end(); ++optionsIterator)
		{
			if (configurationKeys.contains(optionsIterator.key()))
			{
				configuration.setValue(SettingsManager::getOptionName(optionsIterator.value()), configuration.value(optionsIterator.key()).toString());
				configuration.remove(optionsIterator.key());
			}
		}

		QSettings overrides(SettingsManager::getOverridePath(), QSettings::IniFormat);
		const QStringList overridesGroups(overrides.childGroups());

		for (int i = 0; i < overridesGroups.count(); ++i)
		{
			overrides.beginGroup(overridesGroups.at(i));

			const QStringList overridesKeys(overrides.allKeys());

			for (optionsIterator = optionsMap.begin(); optionsIterator != optionsMap.end(); ++optionsIterator)
			{
				if (overridesKeys.contains(optionsIterator.key()))
				{
					overrides.setValue(SettingsManager::getOptionName(optionsIterator.value()), overrides.value(optionsIterator.key()).toString());
					overrides.remove(optionsIterator.key());
				}
			}

			overrides.endGroup();
		}

		const QStringList sessions(SessionsManager::getSessions());

		for (int i = 0; i < sessions.count(); ++i)
		{
			QFile file(SessionsManager::getSessionPath(sessions.at(i)));

			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QString data(file.readAll());

				for (optionsIterator = optionsMap.begin(); optionsIterator != optionsMap.end(); ++optionsIterator)
				{
					data.replace(QLatin1Char('"') + optionsIterator.key() + QLatin1String("\": "), QLatin1Char('"') + SettingsManager::getOptionName(optionsIterator.value()) + QLatin1String("\": "));
				}

				file.close();

				if (file.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					QTextStream stream(&file);
					stream.setCodec("UTF-8");
					stream << data;

					file.close();
				}
			}
		}

		if (configurationKeys.contains(QLatin1String("Sidebar/PanelsOption")) || configurationKeys.contains(QLatin1String("Sidebar/ShowToggleEdgeOption")) || configurationKeys.contains(QLatin1String("Sidebar/VisibleOption")))
		{
			ToolBarsManager::ToolBarDefinition sidebarDefiniton(ToolBarsManager::getToolBarDefinition(ToolBarsManager::SideBar));
			sidebarDefiniton.currentPanel = configuration.value(QLatin1String("Sidebar/CurrentPanelOption")).toString();
			sidebarDefiniton.normalVisibility = (configuration.value(QLatin1String("Sidebar/VisibleOption")).toBool() ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);
			sidebarDefiniton.panels = configuration.value(QLatin1String("Sidebar/PanelsOption")).toStringList();
			sidebarDefiniton.hasToggle = configuration.value(QLatin1String("Sidebar/ShowToggleEdgeOption")).toBool();

			ToolBarsManager::setToolBar(sidebarDefiniton);
		}
	}

	QString getName() const override
	{
		return QLatin1String("optionsRename");
	}

	QString getTitle() const override
	{
		return QT_TRANSLATE_NOOP("migrations", "Options");
	}

	bool canMigrate() const override
	{
		const QStringList sessions(SessionsManager::getSessions());
		bool needsRename(QFile::exists(SettingsManager::getGlobalPath()) || QFile::exists(SettingsManager::getOverridePath()));

		for (int i = 0; i < sessions.count(); ++i)
		{
			needsRename |= QFile::exists(SessionsManager::getSessionPath(sessions.at(i)));
		}

		return needsRename;
	}
};

class SessionsIniToJsonMigration final : public Migration
{
public:
	SessionsIniToJsonMigration() : Migration()
	{
	}

	void createBackup() const override
	{
		const QString backupPath(createBackupPath(QLatin1String("sessions")));
		const QList<QFileInfo> entries(QDir(SessionsManager::getWritableDataPath(QLatin1String("sessions"))).entryInfoList(QStringList(QLatin1String("*.ini"))));

		for (int i = 0; i < entries.count(); ++i)
		{
			QFile::copy(entries.at(i).absoluteFilePath(), backupPath + entries.at(i).fileName());
		}
	}

	void migrate() const override
	{
		const QList<QFileInfo> entries(QDir(SessionsManager::getWritableDataPath(QLatin1String("sessions"))).entryInfoList(QStringList(QLatin1String("*.ini")), QDir::Files));

		for (int i = 0; i < entries.count(); ++i)
		{
			QSettings sessionData(entries.at(i).absoluteFilePath(), QSettings::IniFormat);
			sessionData.setIniCodec("UTF-8");

			SessionInformation session;
			session.path = entries.at(i).absolutePath() + QDir::separator() + entries.at(i).baseName() + QLatin1String(".json");
			session.title = sessionData.value(QLatin1String("Session/title"), QString()).toString();
			session.index = (sessionData.value(QLatin1String("Session/index"), 1).toInt() - 1);
			session.isClean = sessionData.value(QLatin1String("Session/clean"), true).toBool();

			const int windowsAmount(sessionData.value(QLatin1String("Session/windows"), 0).toInt());
			const int defaultZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());

			for (int j = 1; j <= windowsAmount; ++j)
			{
				const int tabs(sessionData.value(QStringLiteral("%1/Properties/windows").arg(j), 0).toInt());
				SessionMainWindow sessionEntry;
				sessionEntry.geometry = QByteArray::fromBase64(sessionData.value(QStringLiteral("%1/Properties/geometry").arg(j), QString()).toString().toLatin1());
				sessionEntry.index = (sessionData.value(QStringLiteral("%1/Properties/index").arg(j), 1).toInt() - 1);

				for (int k = 1; k <= tabs; ++k)
				{
					const QString state(sessionData.value(QStringLiteral("%1/%2/Properties/state").arg(j).arg(k), QString()).toString());
					const QString searchEngine(sessionData.value(QStringLiteral("%1/%2/Properties/searchEngine").arg(j).arg(k), QString()).toString());
					const QString userAgent(sessionData.value(QStringLiteral("%1/%2/Properties/userAgent").arg(j).arg(k), QString()).toString());
					const QStringList geometry(sessionData.value(QStringLiteral("%1/%2/Properties/geometry").arg(j).arg(k), QString()).toString().split(QLatin1Char(',')));
					const int historyAmount(sessionData.value(QStringLiteral("%1/%2/Properties/history").arg(j).arg(k), 0).toInt());
					const int reloadTime(sessionData.value(QStringLiteral("%1/%2/Properties/reloadTime").arg(j).arg(k), -1).toInt());
					SessionWindow sessionWindow;
					sessionWindow.geometry = ((geometry.count() == 4) ? QRect(geometry.at(0).simplified().toInt(), geometry.at(1).simplified().toInt(), geometry.at(2).simplified().toInt(), geometry.at(3).simplified().toInt()) : QRect());
					sessionWindow.state = ((state == QLatin1String("maximized")) ? MaximizedWindowState : ((state == QLatin1String("minimized")) ? MinimizedWindowState : NormalWindowState));
					sessionWindow.parentGroup = sessionData.value(QStringLiteral("%1/%2/Properties/group").arg(j).arg(k), 0).toInt();
					sessionWindow.historyIndex = (sessionData.value(QStringLiteral("%1/%2/Properties/index").arg(j).arg(k), 1).toInt() - 1);
					sessionWindow.isAlwaysOnTop = sessionData.value(QStringLiteral("%1/%2/Properties/alwaysOnTop").arg(j).arg(k), false).toBool();
					sessionWindow.isPinned = sessionData.value(QStringLiteral("%1/%2/Properties/pinned").arg(j).arg(k), false).toBool();

					if (!searchEngine.isEmpty())
					{
						sessionWindow.options[SettingsManager::Search_DefaultSearchEngineOption] = searchEngine;
					}

					if (!userAgent.isEmpty())
					{
						sessionWindow.options[SettingsManager::Network_UserAgentOption] = userAgent;
					}

					if (reloadTime >= 0)
					{
						sessionWindow.options[SettingsManager::Content_PageReloadTimeOption] = reloadTime;
					}

					for (int l = 1; l <= historyAmount; ++l)
					{
						const QStringList position(sessionData.value(QStringLiteral("%1/%2/History/%3/position").arg(j).arg(k).arg(l), 1).toStringList());
						WindowHistoryEntry historyEntry;
						historyEntry.url = sessionData.value(QStringLiteral("%1/%2/History/%3/url").arg(j).arg(k).arg(l), QString()).toString();
						historyEntry.title = sessionData.value(QStringLiteral("%1/%2/History/%3/title").arg(j).arg(k).arg(l), QString()).toString();
						historyEntry.position = ((position.count() == 2) ? QPoint(position.at(0).simplified().toInt(), position.at(1).simplified().toInt()) : QPoint(0, 0));
						historyEntry.zoom = sessionData.value(QStringLiteral("%1/%2/History/%3/zoom").arg(j).arg(k).arg(l), defaultZoom).toInt();

						sessionWindow.history.append(historyEntry);
					}

					sessionEntry.windows.append(sessionWindow);
				}

				session.windows.append(sessionEntry);
			}

			if (SessionsManager::saveSession(session))
			{
				QFile::remove(entries.at(i).absoluteFilePath());
			}
		}
	}

	QString getName() const override
	{
		return QLatin1String("sessionsIniToJson");
	}

	QString getTitle() const override
	{
		return QT_TRANSLATE_NOOP("migrations", "Sessions");
	}

	bool canMigrate() const override
	{
		return !QDir(SessionsManager::getWritableDataPath(QLatin1String("sessions"))).entryList(QStringList(QLatin1String("*.ini")), QDir::Files).isEmpty();
	}
};

Migration::Migration()
{
}

Migration::~Migration()
{
}

void Migration::createBackup() const
{
}

void Migration::migrate() const
{
}

QString Migration::createBackupPath(const QString &sourcePath)
{
	QString backupPath(SessionsManager::getWritableDataPath(QLatin1String("backups") + QDir::separator() + (sourcePath.isEmpty() ? QLatin1String("other") : sourcePath)) + QDir::separator());
	QString backupName(QDate::currentDate().toString(QLatin1String("yyyyMMdd")));
	int i(1);

	do
	{
		const QString path(backupPath + backupName + ((i > 1) ? QStringLiteral("-%1").arg(i) : QString()) + QDir::separator());

		if (!QFile::exists(path))
		{
			backupPath = path;

			break;
		}

		++i;
	}
	while (true);

	QDir().mkpath(backupPath);

	return backupPath;
}

QString Migration::getName() const
{
	return QString();
}

QString Migration::getTitle() const
{
	return QString();
}

bool Migration::canMigrate() const
{
	return false;
}

bool Migrator::run()
{
	const QVector<Migration*> availableMigrations({new OptionsRenameMigration(), new SessionsIniToJsonMigration()});
	QVector<Migration*> possibleMigrations;
	QStringList processedMigrations(SettingsManager::getOption(SettingsManager::Browser_MigrationsOption).toStringList());

	for (int i = 0; i < availableMigrations.count(); ++i)
	{
		if (!processedMigrations.contains(availableMigrations[i]->getName()) && availableMigrations[i]->canMigrate())
		{
			possibleMigrations.append(availableMigrations[i]);
		}
	}

	if (possibleMigrations.isEmpty())
	{
		return true;
	}

	QDialog dialog;
	dialog.setWindowTitle(QCoreApplication::translate("Otter::Migrator", "Settings Migration"));
	dialog.setLayout(new QVBoxLayout(&dialog));

	QLabel *label(new QLabel(QCoreApplication::translate("Otter::Migrator", "Configuration of the components listed below needs to be updated to new version.\nDo you want to migrate it?"), &dialog));
	label->setWordWrap(true);

	ItemViewWidget *migrationsViewWidget(new ItemViewWidget(&dialog));
	migrationsViewWidget->setModel(new QStandardItemModel(migrationsViewWidget));

	for (int i = 0; i < possibleMigrations.count(); ++i)
	{
		migrationsViewWidget->insertRow(new QStandardItem(QCoreApplication::translate("migrations", possibleMigrations[i]->getTitle().toUtf8().constData())));
	}

	QCheckBox *createBackupCheckBox(new QCheckBox(QCoreApplication::translate("Otter::Migrator", "Create backup")));
	createBackupCheckBox->setChecked(true);

	QDialogButtonBox *buttonBox(new QDialogButtonBox(&dialog));
	buttonBox->addButton(QDialogButtonBox::Yes);
	buttonBox->addButton(QDialogButtonBox::No);
	buttonBox->addButton(QDialogButtonBox::Abort);

	QDialogButtonBox::StandardButton clickedButton(QDialogButtonBox::Yes);

	QObject::connect(buttonBox, &QDialogButtonBox::clicked, [&](QAbstractButton *button)
	{
		clickedButton = buttonBox->standardButton(button);

		dialog.close();
	});

	dialog.layout()->addWidget(label);
	dialog.layout()->addWidget(migrationsViewWidget);
	dialog.layout()->addWidget(createBackupCheckBox);
	dialog.layout()->addWidget(buttonBox);
	dialog.exec();

	const bool canProceed(clickedButton == QDialogButtonBox::Yes);
	const bool needsBackup(createBackupCheckBox->isChecked());

	if (canProceed || needsBackup)
	{
		for (int i = 0; i < possibleMigrations.count(); ++i)
		{
			processedMigrations.append(possibleMigrations[i]->getName());

			if (needsBackup)
			{
				possibleMigrations[i]->createBackup();
			}

			if (canProceed)
			{
				possibleMigrations[i]->migrate();
			}
		}
	}

	qDeleteAll(availableMigrations);

	if (clickedButton == QDialogButtonBox::Abort)
	{
		return false;
	}

	SettingsManager::setValue(SettingsManager::Browser_MigrationsOption, QVariant(processedMigrations));

	return true;
}

}
