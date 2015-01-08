/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ContentBlockingDialog.h"
#include "../OptionDelegate.h"
#include "../../core/ContentBlockingList.h"
#include "../../core/ContentBlockingManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/Utils.h"

#include "ui_ContentBlockingDialog.h"

#include <QtCore/QSettings>
#include <QtGui/QStandardItemModel>

namespace Otter
{

ContentBlockingDialog::ContentBlockingDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::ContentBlockingDialog)
{
	m_ui->setupUi(this);

	const QList<ContentBlockingList*> definitions = ContentBlockingManager::getBlockingDefinitions();
	QStandardItemModel *model = new QStandardItemModel(this);
	QStringList labels;
	labels << tr("Name") << tr("Last update");

	model->setHorizontalHeaderLabels(labels);

	for (int i = 0; i < definitions.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(definitions.at(i)->getListName()));
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		items[0]->setData(definitions.at(i)->getConfigListName(), Qt::UserRole);
		items[0]->setCheckable(true);
		items[0]->setCheckState(definitions.at(i)->isEnabled() ? Qt::Checked : Qt::Unchecked);

		items.append(new QStandardItem(Utils::formatDateTime(definitions.at(i)->getLastUpdate())));
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		model->appendRow(items);
	}

	m_ui->filtersViewWidget->setModel(model);
	m_ui->filtersViewWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->filtersViewWidget->header()->setVisible(true);
	m_ui->filtersViewWidget->setItemDelegate(new OptionDelegate(true, this));

	connect(m_ui->confirmButtonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->confirmButtonBox, SIGNAL(rejected()), this, SLOT(close()));
}

ContentBlockingDialog::~ContentBlockingDialog()
{
	delete m_ui;
}

void ContentBlockingDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void ContentBlockingDialog::save()
{
	QSettings adBlock(SessionsManager::getProfilePath() + QLatin1String("/adblock.ini"), QSettings::IniFormat);
	adBlock.setIniCodec("UTF-8");

	for (int i = 0; i < m_ui->filtersViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->filtersViewWidget->getIndex(i, 0);

		adBlock.setValue(index.data(Qt::UserRole).toString() + QLatin1String("/enabled"), index.data(Qt::CheckStateRole).toBool());
	}

	adBlock.sync();

	ContentBlockingManager::updateLists();

	close();
}

}
