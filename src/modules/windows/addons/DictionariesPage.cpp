/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileDialog>

namespace Otter
{
///TODO
/// allow to disable dictionaries
/// connect to dictionaries changed signal

DictionariesPage::DictionariesPage(bool needsDetails, QWidget *parent) : AddonsPage(needsDetails, parent)
{
}

void DictionariesPage::addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData)
{
	QStandardItem* item(new QStandardItem(getAddonIcon(addon), addon->getTitle()));
	item->setData(getAddonIdentifier(addon), IdentifierRole);
	item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
	item->setCheckable(true);
	item->setCheckState(addon->isEnabled() ? Qt::Checked : Qt::Unchecked);
	item->setToolTip(getAddonToolTip(addon));

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
			model->setData(index, getAddonIcon(addon), Qt::DecorationRole);
			model->setData(index, addon->getTitle(), Qt::DisplayRole);
			model->setData(index, getAddonToolTip(addon), Qt::ToolTipRole);

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
	const QStringList sourcePaths(QFileDialog::getOpenFileNames(this, tr("Select Files"), Utils::getStandardLocation(QStandardPaths::HomeLocation), Utils::formatFileTypes({tr("Hunspell dictionary files (*.aff *.dic)")})));

	if (sourcePaths.count() != 2)
	{
		return;
	}

	QString language;
	bool hasAff(false);

	for (int i = 0; i < sourcePaths.count(); ++i)
	{
		const QFileInfo fileInformation(sourcePaths.at(i));
		const QString suffix(fileInformation.suffix().toLower());

		if (suffix == QLatin1String("aff"))
		{
			hasAff = true;
		}
		else if (suffix == QLatin1String("dic"))
		{
			language = fileInformation.baseName();
		}
	}

	if (hasAff && !language.isEmpty())
	{
		SpellCheckManager::DictionaryInformation dictionary;
		dictionary.language = language;
		dictionary.paths = sourcePaths;

		m_dictionariesToAdd.append(dictionary);
	}
}

void DictionariesPage::openAddons()
{
	const QStringList selectedDictionaries(getSelectedDictionaries());

	for (int i = 0; i < selectedDictionaries.count(); ++i)
	{
		const QStringList paths(SpellCheckManager::getDictionary(selectedDictionaries.at(i)).paths);

		for (int j = 0; j < paths.count(); ++j)
		{
			Utils::runApplication({}, paths.at(j));
		}
	}
}

void DictionariesPage::removeAddons()
{
	const QStringList dictionaries(getSelectedDictionaries());

	if (dictionaries.isEmpty() || !confirmAddonsRemoval(dictionaries.count()))
	{
		return;
	}

	bool hasDictionariesToRemove(false);

	m_filesToRemove.reserve(m_filesToRemove.count() + (dictionaries.count() * 2));

	for (int i = 0; i < dictionaries.count(); ++i)
	{
		const SpellCheckManager::DictionaryInformation dictionary(SpellCheckManager::getDictionary(dictionaries.at(i)));

		if (dictionary.isLocalDictionary)
		{
			m_filesToRemove.append(dictionary.paths);

			hasDictionariesToRemove = true;
		}
	}

	if (hasDictionariesToRemove)
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
		SpellCheckManager::DictionaryInformation dictionary(SpellCheckManager::getDictionary(selectedDictionaries.first()));

		if (dictionary.isValid())
		{
			titleEntry.value = Dictionary(dictionary, this).getTitle();
			codeEntry.value = dictionary.language;
			locationEntry.value = QFileInfo(dictionary.paths.at(0)).absolutePath();
		}
	}

	setDetails({titleEntry, codeEntry, locationEntry});
}

void DictionariesPage::save()
{
	const QString dictionariesPath(SpellCheckManager::getDictionariesPath());
	const QDir dictionariesDirectory(dictionariesPath);

	Utils::removeFiles(m_filesToRemove);

	m_filesToRemove.clear();

	Utils::ensureDirectoryExists(dictionariesPath);

	for (int i = 0; i < m_dictionariesToAdd.count(); ++i)
	{
		const SpellCheckManager::DictionaryInformation dictionary(m_dictionariesToAdd.at(i));

		for (int j = 0; j < dictionary.paths.count(); ++j)
		{
			const QString path(dictionary.paths.at(j));

			QFile::copy(path, dictionariesDirectory.filePath(dictionary.language + QFileInfo(path).suffix()));
		}
	}

	m_dictionariesToAdd.clear();

	QStandardItemModel *model(getModel());
	QStringList disabledDictionaries;

	for (int i = 0; i < model->rowCount(); ++i)
	{
		const QModelIndex index(model->index(i, 0));

		if (static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) != Qt::Checked)
		{
			disabledDictionaries.append(index.data(IdentifierRole).toString());
		}
	}

	SettingsManager::setOption(SettingsManager::Browser_SpellCheckIgnoreDctionariesOption, disabledDictionaries);
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
