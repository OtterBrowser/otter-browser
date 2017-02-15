/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "SessionsManager.h"
#include "Settings.h"
#include "SettingsManager.h"

#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

Migrator::Migrator(QObject *parent) : QObject(parent)
{
	registerMigration(QLatin1String("optionsRename"), QT_TRANSLATE_NOOP("migrations", "Options"));
	registerMigration(QLatin1String("sessionsIniToJson"), QT_TRANSLATE_NOOP("migrations", "Sessions"));
}

void Migrator::run()
{
	QSet<QString> newMigrations;
	newMigrations.insert(QLatin1String("optionsRename"));

	if (!QDir(SessionsManager::getWritableDataPath(QLatin1String("sessions"))).entryList(QStringList(QLatin1String("*.ini")), QDir::Files).isEmpty())
	{
		newMigrations.insert(QLatin1String("sessionsIniToJson"));
	}

	QSet<QString> migrations(SettingsManager::getValue(SettingsManager::Browser_MigrationsOption).toStringList().toSet());

	newMigrations -= migrations;

	if (newMigrations.isEmpty())
	{
		return;
	}

	QSet<QString>::iterator iterator;

	for (iterator = newMigrations.begin(); iterator != newMigrations.end(); ++iterator)
	{
		const MigrationFlags flags(checkMigrationStatus(*iterator));

		if (flags == NoMigration)
		{
			continue;
		}

		if (flags.testFlag(IgnoreMigration) || flags.testFlag(ProceedMigration))
		{
			migrations.insert(*iterator);
		}

		if (flags.testFlag(WithBackupMigration))
		{
			createBackup(*iterator);
		}

		if (!flags.testFlag(ProceedMigration))
		{
			continue;
		}

		if (*iterator == QLatin1String("optionsRename"))
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
			optionsMap[QLatin1String("Content/PopupsPolicy")] = SettingsManager::Permissions_ScriptsCanOpenWindowsOption;
			optionsMap[QLatin1String("Browser/JavaScriptCanDisableContextMen")] = SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption;
			optionsMap[QLatin1String("Browser/JavaScriptCanShowStatusMessages")] = SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption;

			QMap<QString, SettingsManager::OptionIdentifier>::iterator optionsIterator;
			QSettings configuration(SessionsManager::getWritableDataPath(QLatin1String("otter.conf")), QSettings::IniFormat);
			const QStringList configurationKeys(configuration.allKeys());

			for (optionsIterator = optionsMap.begin(); optionsIterator != optionsMap.end(); ++optionsIterator)
			{
				if (configurationKeys.contains(optionsIterator.key()))
				{
					configuration.setValue(SettingsManager::getOptionName(optionsIterator.value()), configuration.value(optionsIterator.key()).toString());
					configuration.remove(optionsIterator.key());
				}
			}

			QSettings overrides(SessionsManager::getWritableDataPath(QLatin1String("override.ini")), QSettings::IniFormat);
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
						stream << data;

						file.close();
					}
				}
			}
		}

		if (*iterator == QLatin1String("sessionsIniToJson"))
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
				const int defaultZoom(SettingsManager::getValue(SettingsManager::Content_DefaultZoomOption).toInt());

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
							sessionWindow.overrides[SettingsManager::Search_DefaultSearchEngineOption] = searchEngine;
						}

						if (!userAgent.isEmpty())
						{
							sessionWindow.overrides[SettingsManager::Network_UserAgentOption] = userAgent;
						}

						if (reloadTime >= 0)
						{
							sessionWindow.overrides[SettingsManager::Content_PageReloadTimeOption] = reloadTime;
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
	}

	SettingsManager::setValue(SettingsManager::Browser_MigrationsOption, QVariant(migrations.toList()));
}

void Migrator::createBackup(const QString &identifier)
{
	QString sourcePath;
	QStringList sourceFilters;

	if (identifier == QLatin1String("optionsRename"))
	{
		sourceFilters = QStringList({QLatin1String("otter.conf"), QLatin1String("override.ini")});
	}
	else if (identifier == QLatin1String("sessionsIniToJson"))
	{
		sourcePath = QLatin1String("sessions");
		sourceFilters = QStringList(QLatin1String("*.ini"));
	}

	if (sourcePath.isEmpty() && sourceFilters.isEmpty())
	{
		return;
	}

	QString targetPath(SessionsManager::getWritableDataPath(QLatin1String("backups") + QDir::separator() + (sourcePath.isEmpty() ? QLatin1String("other") : sourcePath)) + QDir::separator());
	QString backupName(QDate::currentDate().toString(QLatin1String("yyyyMMdd")));
	int i(1);

	do
	{
		const QString path(targetPath + backupName + ((i > 1) ? QStringLiteral("-%1").arg(i) : QString()) + QDir::separator());

		if (!QFile::exists(path))
		{
			targetPath = path;

			break;
		}

		++i;
	}
	while (true);

	QDir().mkpath(targetPath);

	const QList<QFileInfo> entries(QDir(SessionsManager::getWritableDataPath(sourcePath)).entryInfoList(sourceFilters));

	for (int i = 0; i < entries.count(); ++i)
	{
		QFile::copy(entries.at(i).absoluteFilePath(), targetPath + entries.at(i).fileName());
	}
}

void Migrator::registerMigration(const QString &identifier, const QString &title)
{
	m_migrations[identifier] = title;
}

Migrator::MigrationFlags Migrator::checkMigrationStatus(const QString &identifier) const
{
	if (!m_migrations.contains(identifier))
	{
		return NoMigration;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Configuration of %1 needs to be updated to new version.\nDo you want to migrate it?").arg(QCoreApplication::translate("migrations", m_migrations[identifier].toUtf8().constData())));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setCheckBox(new QCheckBox(tr("Create backup")));
	messageBox.addButton(tr("Yes"), QMessageBox::AcceptRole);

	QAbstractButton *ignoreButton(messageBox.addButton(tr("No"), QMessageBox::RejectRole));
	QAbstractButton *cancelButton(messageBox.addButton(tr("Cancel"), QMessageBox::RejectRole));

	messageBox.checkBox()->setChecked(true);
	messageBox.exec();

	if (messageBox.clickedButton() == cancelButton)
	{
		return NoMigration;
	}

	MigrationFlags flags(NoMigration);

	if (messageBox.clickedButton() == ignoreButton)
	{
		flags |= IgnoreMigration;
	}
	else
	{
		flags |= ProceedMigration;
	}

	if (messageBox.checkBox()->isChecked())
	{
		flags |= WithBackupMigration;
	}

	return flags;
}

}
