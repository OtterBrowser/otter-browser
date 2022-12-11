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

#include "DictionariesPage.h"
#include "../../../core/SpellCheckManager.h"
#include "../../../ui/ItemViewWidget.h"

namespace Otter
{

DictionariesPage::DictionariesPage(bool needsDetails, QWidget *parent) : AddonsPage(needsDetails, parent)
{
	getModel()->setHorizontalHeaderLabels({tr("Title")});

	connect(this, &AddonsPage::needsActionsUpdate, this, [&]()
	{
		const QModelIndexList indexes(getSelectedIndexes());

 		if (indexes.isEmpty())
 		{
 			emit currentAddonChanged(nullptr);
 		}
 		else
 		{
			SpellCheckManager::DictionaryInformation information(SpellCheckManager::getDictionary(indexes.first().data(IdentifierRole).toString()));

			if (information.isValid())
			{
				Dictionary dictionary(information, this);

				emit currentAddonChanged(&dictionary);
			}
			else
			{
				emit currentAddonChanged(nullptr);
			}
 		}
	});
}

void DictionariesPage::addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData)
{
	QStandardItem* item(new QStandardItem((addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), addon->getTitle()));
	item->setData(getAddonIdentifier(addon), IdentifierRole);
	item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
	item->setCheckable(true);
	item->setCheckState(addon->isEnabled() ? Qt::Checked : Qt::Unchecked);
	item->setToolTip(addon->getDescription());

	getModel()->appendRow(item);
	getModel()->setItemData(item->index(), metaData);
}

void DictionariesPage::updateAddonEntry(Addon *addon)
{
	const QVariant identifier(getAddonIdentifier(addon));

	for (int i = 0; i < getModel()->rowCount(); ++i)
	{
		const QModelIndex index(getModel()->index(i, 0));

		if (index.data(IdentifierRole) == identifier)
		{
			getModel()->setData(index, (addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), Qt::DecorationRole);
			getModel()->setData(index, addon->getTitle(), Qt::DisplayRole);
			getModel()->setData(index, addon->getDescription(), Qt::ToolTipRole);

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

}

void DictionariesPage::openAddons()
{

}

void DictionariesPage::reloadAddons()
{

}

void DictionariesPage::removeAddons()
{

}

void DictionariesPage::updateDetails()
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

	DetailsEntry titleEntry;
	titleEntry.label = tr("Title:");

	if (selectedDictionaries.count() == 1)
	{
		SpellCheckManager::DictionaryInformation information(SpellCheckManager::getDictionary(selectedDictionaries.first()));

		if (information.isValid())
		{
			titleEntry.value = Dictionary(information, this).getTitle();
		}
	}

	setDetails({titleEntry});
}

void DictionariesPage::save()
{

}

QString DictionariesPage::getTitle() const
{
	return tr("Dictionaries");
}

QVariant DictionariesPage::getAddonIdentifier(Addon *addon) const
{
	return addon->getName();
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

bool DictionariesPage::canReloadAddons() const
{
	return true;
}

}
