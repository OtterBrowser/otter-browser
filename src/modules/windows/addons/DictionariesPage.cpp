/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "DictionariesPage.h"
#include "../../../ui/ItemViewWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QFileDialog>

namespace Otter
{

DictionariesPage::DictionariesPage(bool needsDetails, QWidget *parent) : AddonsPage(needsDetails, parent)
{
}

void DictionariesPage::addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData)
{
	QStandardItem* item(new QStandardItem((addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), addon->getTitle()));
	item->setData(getAddonIdentifier(addon), IdentifierRole);
	item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
	item->setCheckable(true);
	item->setCheckState(addon->isEnabled() ? Qt::Checked : Qt::Unchecked);
	item->setToolTip(addon->getDescription());

	QStandardItemModel *model(getModel());
	model->appendRow(item);
	model->setItemData(item->index(), metaData);
}

void DictionariesPage::updateAddonEntry(Addon *addon)
{
	QStandardItemModel *model(getModel());
	const QVariant identifier(getAddonIdentifier(addon));

	for (int i = 0; i < model->rowCount(); ++i)
	{
		const QModelIndex index(model->index(i, 0));

		if (index.data(IdentifierRole) == identifier)
		{
			model->setData(index, (addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), Qt::DecorationRole);
			model->setData(index, addon->getTitle(), Qt::DisplayRole);
			model->setData(index, addon->getDescription(), Qt::ToolTipRole);

			return;
		}
	}
}

void DictionariesPage::delayedLoad()
{
	const QVector<SpellCheckManager::DictionaryInformation> dictionaries(SpellCheckManager::getDictionaries());

	for (int i = 0; i < dictionaries.count(); ++i)
	{
		Dictionary dictionary(dictionaries.at(i), this);

		addAddonEntry(&dictionary);
	}
}

void DictionariesPage::addAddon()
{
	const QStringList sourcePaths(QFileDialog::getOpenFileNames(this, tr("Select Files"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), Utils::formatFileTypes({tr("Hunspell dictionary files (*.aff *.dic)")})));

	if (sourcePaths.count() != 2)
	{
		return;
	}

///TODO
}

void DictionariesPage::openAddons()
{
	const QStringList selectedDictionaries(getSelectedDictionaries());
	QStringList paths;

	for (int i = 0; i < selectedDictionaries.count(); ++i)
	{
		paths.append(SpellCheckManager::getDictionary(selectedDictionaries.at(i)).paths);
	}

	for (int i = 0; i < paths.count(); ++i)
	{
		Utils::runApplication({}, paths.at(i));
	}
}

void DictionariesPage::removeAddons()
{
	const QStringList dictionaries(getSelectedDictionaries());

	if (dictionaries.isEmpty() || !confirmAddonsRemoval(dictionaries.count()))
	{
		return;
	}

	bool hasAddonsToRemove(false);

	for (int i = 0; i < dictionaries.count(); ++i)
	{
		const SpellCheckManager::DictionaryInformation information(SpellCheckManager::getDictionary(dictionaries.at(i)));

		if (information.isLocalDictionary)
		{
			m_filesToRemove.append(information.paths);

			hasAddonsToRemove = true;
		}
	}

	if (hasAddonsToRemove)
	{
		emit settingsModified();
	}
}

void DictionariesPage::updateDetails()
{
	const QStringList selectedDictionaries(getSelectedDictionaries());
	DetailsEntry titleEntry;
	titleEntry.label = tr("Title:");

	DetailsEntry codeEntry;
	codeEntry.label = tr("Language Code:");

	DetailsEntry locationEntry;
	locationEntry.label = tr("Location:");

	if (selectedDictionaries.count() == 1)
	{
		SpellCheckManager::DictionaryInformation information(SpellCheckManager::getDictionary(selectedDictionaries.first()));

		if (information.isValid())
		{
			titleEntry.value = Dictionary(information, this).getTitle();
			codeEntry.value = information.language;
			locationEntry.value = QFileInfo(information.paths.at(0)).absolutePath();
		}
	}

	setDetails({titleEntry, codeEntry, locationEntry});
}

void DictionariesPage::save()
{
	const QString dictionariesPath(SpellCheckManager::getDictionariesPath());
	const QDir dictionariesDirectory(dictionariesPath);

	QDir().mkpath(dictionariesPath);

	for (int i = 0; i < m_filesToRemove.count(); ++i)
	{
		QFile::remove(m_filesToRemove.at(i));
	}

	m_filesToRemove.clear();

	for (int i = 0; i < m_dictionariesToAdd.count(); ++i)
	{
		const QString language(m_dictionariesToAdd.at(i).language);
		const QStringList paths(m_dictionariesToAdd.at(i).paths);

		for (int j = 0; j < paths.count(); ++j)
		{
			const QString path(paths.at(j));

			QFile::copy(path, dictionariesDirectory.filePath(language + QFileInfo(path).suffix()));
		}
	}
}

QString DictionariesPage::getTitle() const
{
	return tr("Dictionaries");
}

QVariant DictionariesPage::getAddonIdentifier(Addon *addon) const
{
	return addon->getName();
}

QStringList DictionariesPage::getSelectedDictionaries() const
{
	const QModelIndexList indexes(getSelectedIndexes());
	QStringList selectedDictionaries;
	selectedDictionaries.reserve(indexes.count());

	for (int i = 0; i < indexes.count(); ++i)
	{
		const QString identifier(indexes.at(i).data(IdentifierRole).toString());

		if (!identifier.isEmpty())
		{
			selectedDictionaries.append(identifier);
		}
	}

	return selectedDictionaries;
}

QVector<AddonsPage::ModelColumn> DictionariesPage::getModelColumns() const
{
	ModelColumn titleColumn;
	titleColumn.label = tr("Title");

	return {titleColumn};
}

bool DictionariesPage::canOpenAddons() const
{
	return true;
}

}
