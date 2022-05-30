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
			addAddon(script, {{NameRole, script->getName()}});
		}
	}
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
		UserScript *script(AddonsManager::getUserScript(indexes.at(i).data(NameRole).toString()));

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
