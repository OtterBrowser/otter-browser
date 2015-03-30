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
#include "ContentBlockingIntervalDelegate.h"
#include "../OptionDelegate.h"
#include "../../core/ContentBlockingManager.h"
#include "../../core/ContentBlockingProfile.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"
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

	const QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	const QStringList globalProfiles = SettingsManager::getValue(QLatin1String("Content/BlockingProfiles")).toStringList();
	const QVector<ContentBlockingInformation> profiles = ContentBlockingManager::getProfiles();
	QStandardItemModel *model = new QStandardItemModel(this);
	QStringList labels;
	labels << tr("Title") << tr("Update Interval") << tr("Last Update");

	model->setHorizontalHeaderLabels(labels);

	for (int i = 0; i < profiles.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(profiles.at(i).title));
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[0]->setData(profiles.at(i).name, Qt::UserRole);
		items[0]->setCheckable(true);
		items[0]->setCheckState(globalProfiles.contains(profiles.at(i).name) ? Qt::Checked : Qt::Unchecked);

		items.append(new QStandardItem(profilesSettings.value(profiles.at(i).name + QLatin1String("/updateInterval")).toString()));
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);

		items.append(new QStandardItem(profilesSettings.value(profiles.at(i).name + QLatin1String("/lastUpdate")).toString()));
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		model->appendRow(items);
	}

	m_ui->profliesViewWidget->setModel(model);
	m_ui->profliesViewWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->profliesViewWidget->header()->setVisible(true);
	m_ui->profliesViewWidget->setItemDelegate(new OptionDelegate(true, this));
	m_ui->profliesViewWidget->setItemDelegateForColumn(1, new ContentBlockingIntervalDelegate(this));

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
	QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	QStringList profiles;

	for (int i = 0; i < m_ui->profliesViewWidget->getRowCount(); ++i)
	{
		profilesSettings.beginGroup(m_ui->profliesViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());
		profilesSettings.setValue(QLatin1String("lastUpdate"), m_ui->profliesViewWidget->getIndex(i, 2).data(Qt::DisplayRole));
		profilesSettings.setValue(QLatin1String("updateInterval"), m_ui->profliesViewWidget->getIndex(i, 1).data(Qt::DisplayRole));
		profilesSettings.endGroup();

		if (m_ui->profliesViewWidget->getIndex(i, 0).data(Qt::CheckStateRole).toBool())
		{
			profiles.append(m_ui->profliesViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());
		}
	}

	SettingsManager::setValue(QLatin1String("Content/BlockingProfiles"), profiles);

	close();
}

}
