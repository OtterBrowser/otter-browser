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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

Migrator::Migrator(QObject *parent) : QObject(parent)
{
}

void Migrator::run()
{
	QSet<QString> newMigrations;

///TODO

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

///TODO
	}

	SettingsManager::setValue(SettingsManager::Browser_MigrationsOption, QVariant(migrations.toList()));
}

void Migrator::createBackup(const QString &identifier)
{
	QString sourcePath;
	QStringList sourceFilters(QLatin1String("*"));

///TODO

	if (sourcePath.isEmpty())
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
