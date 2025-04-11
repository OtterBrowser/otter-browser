/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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
#include "../../../core/JsonSettings.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/UserScript.h"

#include <QtCore/QJsonObject>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

UserScriptsPage::UserScriptsPage(bool needsDetails, QWidget *parent) : AddonsPage(needsDetails, parent)
{
	connect(AddonsManager::getInstance(), &AddonsManager::userScriptModified, this, [&](const QString &name)
	{
		Addon *addon(AddonsManager::getUserScript(name));

		if (addon)
		{
			updateAddonEntry(addon);
		}
	});
}

void UserScriptsPage::delayedLoad()
{
	const QStringList userScripts(AddonsManager::getAddons(Addon::UserScriptType));

	for (int i = 0; i < userScripts.count(); ++i)
	{
		UserScript *script(AddonsManager::getUserScript(userScripts.at(i)));

		if (script)
		{
			addAddonEntry(script);
		}
	}
}

void UserScriptsPage::addAddon()
{
	const QStringList sourcePaths(QFileDialog::getOpenFileNames(this, tr("Select Files"), Utils::getStandardLocation(QStandardPaths::HomeLocation), Utils::formatFileTypes({tr("User Script files (*.js)")})));

	if (sourcePaths.isEmpty())
	{
		return;
	}

	QStringList failedPaths;
	ReplaceMode replaceMode(UnknownMode);

	for (int i = 0; i < sourcePaths.count(); ++i)
	{
		const QString sourcePath(sourcePaths.at(i));

		if (sourcePath.isEmpty())
		{
			continue;
		}

		const QString scriptName(QFileInfo(sourcePath).completeBaseName());
		const QString targetDirectory(QDir(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).filePath(scriptName));
		const QString targetPath(QDir(targetDirectory).filePath(QFileInfo(sourcePath).fileName()));
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

		if ((isReplacingScript && !QDir().remove(targetPath)) || (!isReplacingScript && !Utils::ensureDirectoryExists(targetDirectory)) || !QFile::copy(sourcePath, targetPath))
		{
			failedPaths.append(sourcePath);

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

///TODO apply changes later, take removed addons into account?
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
		UserScript *addon(addons.at(i));

		addon->reload();

		updateAddonEntry(addon);
	}
}

void UserScriptsPage::removeAddons()
{
	const QVector<UserScript*> addons(getSelectedUserScripts());

	if (addons.isEmpty() || !confirmAddonsRemoval(addons.count()))
	{
		return;
	}

	bool hasAddonsToRemove(false);

	for (int i = 0; i < addons.count(); ++i)
	{
		UserScript *addon(addons.at(i));

		if (addon->canRemove())
		{
			m_addonsToRemove.append(addon->getName());

			hasAddonsToRemove = true;
		}
	}

	if (hasAddonsToRemove)
	{
		emit settingsModified();
	}
}

void UserScriptsPage::updateDetails()
{
	const QVector<UserScript*> selectedUserScripts(getSelectedUserScripts());
	DetailsEntry titleEntry;
	titleEntry.label = tr("Title:");

	DetailsEntry homePageEntry;
	homePageEntry.label = tr("Homepage:");

	if (selectedUserScripts.count() == 1)
	{
		UserScript *script(selectedUserScripts.first());

		if (script)
		{
			const QUrl homePage(script->getHomePage());

			titleEntry.value = script->getTitle();
			homePageEntry.value = homePage.toDisplayString();
			homePageEntry.isUrl = !homePage.isEmpty();
		}
	}

	setDetails({titleEntry, homePageEntry});
}

void UserScriptsPage::save()
{
	for (int i = 0; i < m_addonsToRemove.count(); ++i)
	{
		Addon *addon(AddonsManager::getUserScript(m_addonsToRemove.at(i)));

		if (addon)
		{
			addon->remove();
		}
	}

	QStandardItemModel *model(getModel());
	QModelIndexList indexesToRemove;
	QJsonObject settingsObject;

	for (int i = 0; i < model->rowCount(); ++i)
	{
		const QModelIndex index(model->index(i, 0));

		if (index.isValid())
		{
			const QString name(index.data(IdentifierRole).toString());

			if (!name.isEmpty() && AddonsManager::getUserScript(name))
			{
				const bool isEnabled(static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) == Qt::Checked);

				settingsObject.insert(name, QJsonObject({{QLatin1String("isEnabled"), QJsonValue(isEnabled)}}));
			}
			else
			{
				indexesToRemove.append(index);
			}
		}
	}

	JsonSettings settings;
	settings.setObject(settingsObject);
	settings.save(SessionsManager::getWritableDataPath(QLatin1String("scripts/scripts.json")));

	for (int i = (indexesToRemove.count() - 1); i >= 0; --i)
	{
		const QModelIndex index(indexesToRemove.at(i));

		model->removeRow(index.row(), index.parent());
	}

	m_addonsToAdd.clear();
	m_addonsToRemove.clear();
}

QString UserScriptsPage::getTitle() const
{
	return tr("User Scripts");
}

QIcon UserScriptsPage::getFallbackIcon() const
{
	return ThemesManager::createIcon(QLatin1String("addon-user-script"), false);
}

QVariant UserScriptsPage::getAddonIdentifier(Addon *addon) const
{
	return addon->getName();
}

QVector<UserScript*> UserScriptsPage::getSelectedUserScripts() const
{
	const QModelIndexList indexes(getSelectedIndexes());
	QVector<UserScript*> userScripts;
	userScripts.reserve(indexes.count());

	for (int i = 0; i < indexes.count(); ++i)
	{
		const QString identifier(indexes.at(i).data(IdentifierRole).toString());

		if (!identifier.isEmpty())
		{
			UserScript *script(AddonsManager::getUserScript(identifier));

			if (script)
			{
				userScripts.append(script);
			}
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
