/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "FeedsComboBoxWidget.h"
#include "ItemViewWidget.h"
#include "../core/FeedsManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QInputDialog>

namespace Otter
{

FeedsComboBoxWidget::FeedsComboBoxWidget(QWidget *parent) : ComboBoxWidget(parent)
{
	setModel(FeedsManager::getModel());
	updateBranch();

	connect(FeedsManager::getModel(), &FeedsModel::layoutChanged, this, &FeedsComboBoxWidget::handleLayoutChanged);
}

void FeedsComboBoxWidget::createFolder()
{
	const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select name of new folder:")));

	if (!title.isEmpty())
	{
		setCurrentFolder(FeedsManager::getModel()->addEntry(FeedsModel::FolderEntry, {{FeedsModel::TitleRole, title}}, getCurrentFolder()));
	}
}

void FeedsComboBoxWidget::handleLayoutChanged()
{
	updateBranch();
}

void FeedsComboBoxWidget::updateBranch(const QModelIndex &parent)
{
	for (int i = 0; i < FeedsManager::getModel()->rowCount(parent); ++i)
	{
		const QModelIndex index(FeedsManager::getModel()->index(i, 0, parent));

		if (index.isValid())
		{
			const FeedsModel::EntryType type(static_cast<FeedsModel::EntryType>(index.data(FeedsModel::TypeRole).toInt()));

			if (type == FeedsModel::RootEntry || type == FeedsModel::FolderEntry)
			{
				updateBranch(index);
			}
			else
			{
				getView()->setRowHidden(i, parent, true);
			}
		}
	}

	if (!parent.isValid())
	{
		getView()->expandAll();
	}
}

void FeedsComboBoxWidget::setCurrentFolder(FeedsModel::Entry *folder)
{
	if (folder)
	{
		setCurrentIndex(folder->index());
	}
}

FeedsModel::Entry* FeedsComboBoxWidget::getCurrentFolder() const
{
	FeedsModel::Entry *item(FeedsManager::getModel()->getEntry(currentData(FeedsModel::IdentifierRole).toULongLong()));

	return (item ? item : FeedsManager::getModel()->getRootEntry());
}

}
