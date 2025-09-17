/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Application.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "../ui/ItemViewWidget.h"

#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

Migration::Migration() = default;

Migration::~Migration() = default;

void Migration::createBackup() const
{
}

void Migration::migrate() const
{
}

QString Migration::createBackupPath(const QString &sourcePath)
{
	QString backupPath(SessionsManager::getWritableDataPath(QLatin1String("backups") + QDir::separator() + (sourcePath.isEmpty() ? QLatin1String("other") : sourcePath)) + QDir::separator());
	const QString backupName(QDate::currentDate().toString(QLatin1String("yyyyMMdd")));
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

	Utils::ensureDirectoryExists(backupPath);

	return backupPath;
}

QString Migration::getName() const
{
	return {};
}

QString Migration::getTitle() const
{
	return {};
}

bool Migration::needsBackup() const
{
	return true;
}

bool Migration::needsMigration() const
{
	return false;
}

bool Migrator::run()
{
	const QVector<Migration*> availableMigrations;
	QVector<Migration*> possibleMigrations;
	QStringList processedMigrations(SettingsManager::getOption(SettingsManager::Browser_MigrationsOption).toStringList());

	for (int i = 0; i < availableMigrations.count(); ++i)
	{
		Migration *availableMigration(availableMigrations.at(i));

		if (!processedMigrations.contains(availableMigration->getName()))
		{
			if (Application::isFirstRun() || !availableMigration->needsMigration())
			{
				processedMigrations.append(availableMigration->getName());
			}
			else
			{
				possibleMigrations.append(availableMigration);
			}
		}
	}

	if (possibleMigrations.isEmpty())
	{
		SettingsManager::setOption(SettingsManager::Browser_MigrationsOption, processedMigrations);

		return true;
	}

	QDialog dialog;
	dialog.setWindowTitle(QCoreApplication::translate("Otter::Migrator", "Settings Migration"));
	dialog.setLayout(new QVBoxLayout(&dialog));

	QLabel *label(new QLabel(QCoreApplication::translate("Otter::Migrator", "Configuration of the components listed below needs to be updated to new version.\nDo you want to migrate it?"), &dialog));
	label->setWordWrap(true);

	ItemViewWidget *migrationsViewWidget(new ItemViewWidget(&dialog));
	migrationsViewWidget->setModel(new QStandardItemModel(migrationsViewWidget));
	migrationsViewWidget->setHeaderHidden(true);
	migrationsViewWidget->header()->setStretchLastSection(true);

	bool needsBackup(false);

	for (int i = 0; i < possibleMigrations.count(); ++i)
	{
		Migration *possibleMigration(possibleMigrations.at(i));
		QStandardItem *item(new QStandardItem(QCoreApplication::translate("migrations", possibleMigration->getTitle().toUtf8().constData())));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable);

		if (possibleMigration->needsBackup())
		{
			needsBackup = true;
		}

		migrationsViewWidget->insertRow({item});
	}

	QCheckBox *createBackupCheckBox(new QCheckBox(QCoreApplication::translate("Otter::Migrator", "Create backup")));
	createBackupCheckBox->setChecked(true);
	createBackupCheckBox->setEnabled(needsBackup);

	QDialogButtonBox *buttonBox(new QDialogButtonBox(&dialog));
	buttonBox->addButton(QDialogButtonBox::Yes);
	buttonBox->addButton(QDialogButtonBox::No);
	buttonBox->addButton(QDialogButtonBox::Abort);

	QDialogButtonBox::StandardButton clickedButton(QDialogButtonBox::Yes);

	QObject::connect(buttonBox, &QDialogButtonBox::clicked, &dialog, [&](QAbstractButton *button)
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

	if (canProceed || createBackupCheckBox->isChecked())
	{
		processedMigrations.reserve(possibleMigrations.count());

		for (int i = 0; i < possibleMigrations.count(); ++i)
		{
			Migration *possibleMigration(possibleMigrations.at(i));

			processedMigrations.append(possibleMigration->getName());

			if (createBackupCheckBox->isChecked())
			{
				possibleMigration->createBackup();
			}

			if (canProceed)
			{
				possibleMigration->migrate();
			}
		}
	}

	qDeleteAll(availableMigrations);

	if (clickedButton == QDialogButtonBox::Abort)
	{
		return false;
	}

	SettingsManager::setOption(SettingsManager::Browser_MigrationsOption, QVariant(processedMigrations));

	return true;
}

}
