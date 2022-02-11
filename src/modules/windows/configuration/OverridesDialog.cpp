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

#include "OverridesDialog.h"
#include "ConfigurationContentsWidget.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SettingsManager.h"

#include "ui_OverridesDialog.h"

#include <QtWidgets/QInputDialog>

namespace Otter
{

OverridesDialog::OverridesDialog(int identifier, QWidget *parent) : Dialog(parent),
	m_model(new QStandardItemModel(this)),
	m_identifier(identifier),
	m_ui(new Ui::OverridesDialog)
{
	m_ui->setupUi(this);

	setWindowTitle(tr("Overrides for %1").arg(SettingsManager::getOptionName(identifier)));

	const QStringList overrideHosts(SettingsManager::getOverrideHosts(identifier));

	for (int i = 0; i < overrideHosts.count(); ++i)
	{
		const QVariant value(SettingsManager::getOption(identifier, overrideHosts.at(i)));
		QList<QStandardItem*> items({new QStandardItem(HistoryManager::getIcon(overrideHosts.at(i)), overrideHosts.at(i)), new QStandardItem()});
		items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
		items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
		items[1]->setData(value, Qt::EditRole);
		items[1]->setData(identifier, ConfigurationContentsWidget::IdentifierRole);

		m_model->appendRow(items);
	}

	m_model->setHorizontalHeaderLabels({tr("Host"), tr("Value")});
	m_model->sort(0);

	m_ui->overridesViewWidget->setViewMode(ItemViewWidget::ListView);
	m_ui->overridesViewWidget->setModel(m_model);
	m_ui->overridesViewWidget->setLayoutDirection(Qt::LeftToRight);
	m_ui->overridesViewWidget->setItemDelegateForColumn(1, new ConfigurationOptionDelegate(false, this));

	connect(m_ui->overridesFilterLineEditWidget, &LineEditWidget::textChanged, m_ui->overridesViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->overridesViewWidget, &ItemViewWidget::needsActionsUpdate, this, [&]()
	{
		m_ui->removeOverrideButton->setEnabled(m_ui->overridesViewWidget->getCurrentIndex().isValid());
	});
	connect(m_ui->overridesViewWidget, &ItemViewWidget::clicked, this, [&](const QModelIndex &index)
	{
		m_ui->overridesViewWidget->setCurrentIndex(index.sibling(index.row(), 1));
	});
	connect(m_ui->overridesViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, [&](const QModelIndex &currentIndex, const QModelIndex &previousIndex)
	{
		if (previousIndex.column() == 1)
		{
			m_ui->overridesViewWidget->closePersistentEditor(previousIndex);
		}

		if (currentIndex.column() == 1)
		{
			m_ui->overridesViewWidget->openPersistentEditor(currentIndex);
		}
	});
	connect(m_ui->addOverrideButton, &QPushButton::clicked, this, [&]()
	{
		const QString host(QInputDialog::getText(this, tr("Select Website Name"), tr("Enter website name:")));

		if (!host.isEmpty())
		{
			for (int i = 0; i < m_model->rowCount(); ++i)
			{
				if (m_ui->overridesViewWidget->getIndex(i, 0).data().toString() == host)
				{
					return;
				}
			}

			QList<QStandardItem*> items({new QStandardItem(HistoryManager::getIcon(host), host), new QStandardItem()});
			items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
			items[0]->setData(true, ConfigurationContentsWidget::IsModifiedRole);
			items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
			items[1]->setData(SettingsManager::getOption(m_identifier), Qt::EditRole);
			items[1]->setData(identifier, ConfigurationContentsWidget::IdentifierRole);

			m_ui->overridesViewWidget->insertRow(items);
		}
	});
	connect(m_ui->removeOverrideButton, &QPushButton::clicked, this, [&]()
	{
		const QModelIndex index(m_ui->overridesViewWidget->getCurrentIndex());

		if (index.isValid())
		{
			m_hostsToRemove.append(index.data(Qt::DisplayRole).toString());

			m_ui->overridesViewWidget->removeRow();
		}
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [&]()
	{
		for (int i = 0; i < m_hostsToRemove.count(); ++i)
		{
			SettingsManager::removeOverride(m_hostsToRemove.at(i), m_identifier);
		}

		if (m_ui->overridesViewWidget->isModified())
		{
			for (int i = 0; i < m_ui->overridesViewWidget->getRowCount(); ++i)
			{
				const QModelIndex index(m_ui->overridesViewWidget->getIndex(i));

				if (index.data(ConfigurationContentsWidget::IsModifiedRole).toBool())
				{
					SettingsManager::setOption(m_identifier, index.sibling(i, 1).data(Qt::EditRole), index.data(Qt::DisplayRole).toString());
				}
			}
		}

		accept();
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &OverridesDialog::close);
}

OverridesDialog::~OverridesDialog()
{
	delete m_ui;
}

void OverridesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_model->setHorizontalHeaderLabels({tr("Host"), tr("Value")});
	}
}

}
