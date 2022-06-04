/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "UserScriptsPage.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/UserScript.h"

#include <QtCore/QStandardPaths>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

UserScriptsPage::UserScriptsPage(QWidget *parent) : AddonsPage(parent)
{
}

void UserScriptsPage::delayedLoad()
{
	const QStringList userScripts(AddonsManager::getAddons(Addon::UserScriptType));

	for (int i = 0; i < userScripts.count(); ++i)
	{
		UserScript *script(AddonsManager::getUserScript(userScripts.at(i)));

		if (script)
		{
			addAddonEntry(script, {{IdentifierRole, script->getName()}});
		}
	}
}

void UserScriptsPage::addAddon()
{
	const QStringList sourcePaths(QFileDialog::getOpenFileNames(this, tr("Select Files"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), Utils::formatFileTypes({tr("User Script files (*.js)")})));

	if (sourcePaths.isEmpty())
	{
		return;
	}

	QStringList failedPaths;
	ReplaceMode replaceMode(UnknownMode);

	for (int i = 0; i < sourcePaths.count(); ++i)
	{
		if (sourcePaths.at(i).isEmpty())
		{
			continue;
		}

		const QString scriptName(QFileInfo(sourcePaths.at(i)).completeBaseName());
		const QString targetDirectory(QDir(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).filePath(scriptName));
		const QString targetPath(QDir(targetDirectory).filePath(QFileInfo(sourcePaths.at(i)).fileName()));
		bool isReplacingScript(false);

		if (QFile::exists(targetPath))
		{
			if (replaceMode == IgnoreAllMode)
			{
				continue;
			}

			if (replaceMode == ReplaceAllMode)
			{
				isReplacingScript = true;
			}
			else
			{
				QMessageBox messageBox;
				messageBox.setWindowTitle(tr("Question"));
				messageBox.setText(tr("User Script with this name already exists:\n%1").arg(targetPath));
				messageBox.setInformativeText(tr("Do you want to replace it?"));
				messageBox.setIcon(QMessageBox::Question);
				messageBox.addButton(QMessageBox::Yes);
				messageBox.addButton(QMessageBox::No);

				if (i < (sourcePaths.count() - 1))
				{
					messageBox.setCheckBox(new QCheckBox(tr("Apply to all")));
				}

				messageBox.exec();

				isReplacingScript = (messageBox.standardButton(messageBox.clickedButton()) == QMessageBox::Yes);

				if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
				{
					replaceMode = (isReplacingScript ? ReplaceAllMode : IgnoreAllMode);
				}
			}

			if (!isReplacingScript)
			{
				continue;
			}
		}

		if ((isReplacingScript && !QDir().remove(targetPath)) || (!isReplacingScript && !QDir().mkpath(targetDirectory)) || !QFile::copy(sourcePaths.at(i), targetPath))
		{
			failedPaths.append(sourcePaths.at(i));

			continue;
		}

		if (isReplacingScript)
		{
			UserScript *script(AddonsManager::getUserScript(scriptName));

			if (script)
			{
				script->reload();
			}
		}
		else
		{
			UserScript script(targetPath);

			addAddonEntry(&script, {{IdentifierRole, script.getName()}});
		}
	}

	if (!failedPaths.isEmpty())
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to import following User Script file(s):\n%1", "", failedPaths.count()).arg(failedPaths.join(QLatin1Char('\n'))), QMessageBox::Close);
	}

	save();

	AddonsManager::loadUserScripts();
}

void UserScriptsPage::openAddons()
{
	const QVector<UserScript*> addons(getSelectedUserScripts());

	for (int i = 0; i < addons.count(); ++i)
	{
		Utils::runApplication({}, addons.at(i)->getPath());
	}
}

void UserScriptsPage::reloadAddons()
{
	const QVector<UserScript*> addons(getSelectedUserScripts());

	for (int i = 0; i < addons.count(); ++i)
	{
		addons.at(i)->reload();
	}
}

void UserScriptsPage::removeAddons()
{
	const QVector<UserScript*> addons(getSelectedUserScripts());

	if (addons.isEmpty())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to irreversibly remove %n addon(s).", "", addons.count()));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		for (int i = 0; i < addons.count(); ++i)
		{
			if (addons.at(i)->canRemove())
			{
				addons.at(i)->remove();
			}
		}
	}

	AddonsManager::loadUserScripts();

	save();
}

QString UserScriptsPage::getTitle() const
{
	return tr("User Scripts");
}

QIcon UserScriptsPage::getFallbackIcon() const
{
	return ThemesManager::createIcon(QLatin1String("addon-user-script"), false);
}

QVector<UserScript*> UserScriptsPage::getSelectedUserScripts() const
{
	const QModelIndexList indexes(getSelectedIndexes());
	QVector<UserScript*> userScripts;
	userScripts.reserve(indexes.count());

	for (int i = 0; i < indexes.count(); ++i)
	{
		UserScript *script(AddonsManager::getUserScript(indexes.at(i).data(IdentifierRole).toString()));

		if (script)
		{
			userScripts.append(script);
		}
	}

	userScripts.squeeze();

	return userScripts;
}

bool UserScriptsPage::canOpenAddons() const
{
	return true;
}

bool UserScriptsPage::canReloadAddons() const
{
	return true;
}

}
