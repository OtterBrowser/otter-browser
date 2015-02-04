/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "../../core/ContentBlockingManager.h"
#include "../../core/ContentBlockingProfile.h"
#include "../../core/SettingsManager.h"
#include "../../core/Utils.h"

#include "ui_ContentBlockingDialog.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

ContentBlockingDialog::ContentBlockingDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::ContentBlockingDialog)
{
	m_ui->setupUi(this);

	const QStringList globalProfiles = SettingsManager::getValue(QLatin1String("Content/BlockingProfiles")).toStringList();
	const QVector<ContentBlockingInformation> profiles = ContentBlockingManager::getProfiles();
	QStandardItemModel *model = new QStandardItemModel(this);
	QStringList labels;
	labels << tr("Title") << tr("Identifier") << tr("Last Update");

	model->setHorizontalHeaderLabels(labels);

	for (int i = 0; i < profiles.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(profiles.at(i).title));
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		items[0]->setCheckable(true);
		items[0]->setCheckState(globalProfiles.contains(profiles.at(i).name) ? Qt::Checked : Qt::Unchecked);

		items.append(new QStandardItem(profiles.at(i).name));
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		items.append(new QStandardItem(Utils::formatDateTime(profiles.at(i).lastUpdate)));
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

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
	QStringList profiles;

	for (int i = 0; i < m_ui->filtersViewWidget->getRowCount(); ++i)
	{
		if (m_ui->filtersViewWidget->getIndex(i, 0).data(Qt::CheckStateRole).toBool())
		{
			profiles.append(m_ui->filtersViewWidget->getIndex(i, 1).data().toString());
		}
	}

	SettingsManager::setValue(QLatin1String("Content/BlockingProfiles"), profiles);

	close();
}

}
