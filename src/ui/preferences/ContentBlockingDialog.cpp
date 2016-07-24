/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

ContentBlockingDialog::ContentBlockingDialog(QWidget *parent) : Dialog(parent),
	m_ui(new Ui::ContentBlockingDialog)
{
	m_ui->setupUi(this);

	const QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	const QStringList globalProfiles(SettingsManager::getValue(QLatin1String("Content/BlockingProfiles")).toStringList());
	const QVector<ContentBlockingProfile*> profiles(ContentBlockingManager::getProfiles());
	QStandardItemModel *model(new QStandardItemModel(this));

	model->setHorizontalHeaderLabels(QStringList({tr("Title"), tr("Update Interval"), tr("Last Update")}));

	for (int i = 0; i < profiles.count(); ++i)
	{
		QList<QStandardItem*> items({new QStandardItem(profiles.at(i)->getTitle()), new QStandardItem(profilesSettings.value(profiles.at(i)->getName() + QLatin1String("/updateInterval")).toString()), new QStandardItem(Utils::formatDateTime(profilesSettings.value(profiles.at(i)->getName() + QLatin1String("/lastUpdate")).toDateTime()))});
		items[0]->setData(profiles.at(i)->getName(), Qt::UserRole);
		items[0]->setData(profiles.at(i)->getUpdateUrl(), (Qt::UserRole + 1));
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[0]->setCheckable(true);
		items[0]->setCheckState(globalProfiles.contains(profiles.at(i)->getName()) ? Qt::Checked : Qt::Unchecked);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		model->appendRow(items);
	}

	m_ui->profilesViewWidget->setModel(model);
	m_ui->profilesViewWidget->setItemDelegate(new OptionDelegate(true, this));
	m_ui->profilesViewWidget->setItemDelegateForColumn(1, new ContentBlockingIntervalDelegate(this));

	connect(ContentBlockingManager::getInstance(), SIGNAL(profileModified(QString)), this, SLOT(profileModified(QString)));
	connect(m_ui->profilesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
	connect(m_ui->updateButton, SIGNAL(clicked(bool)), this, SLOT(updateProfile()));
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

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ContentBlockingDialog::profileModified(const QString &name)
{
	ContentBlockingProfile *profile(ContentBlockingManager::getProfile(name));

	if (!profile)
	{
		return;
	}

	for (int i = 0; i < m_ui->profilesViewWidget->model()->rowCount(); ++i)
	{
		const QModelIndex index(m_ui->profilesViewWidget->model()->index(i, 0));

		if (index.data(Qt::UserRole).toString() == name)
		{
			const QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);

			m_ui->profilesViewWidget->model()->setData(index, profile->getTitle(), Qt::DisplayRole);
			m_ui->profilesViewWidget->model()->setData(index.sibling(i, 2), Utils::formatDateTime(profilesSettings.value(name + QLatin1String("/lastUpdate")).toDateTime()), Qt::DisplayRole);

			break;
		}
	}
}

void ContentBlockingDialog::updateProfile()
{
	const QModelIndex index(m_ui->profilesViewWidget->getIndex(m_ui->profilesViewWidget->getCurrentRow()));

	if (index.isValid())
	{
		ContentBlockingManager::updateProfile(index.data(Qt::UserRole).toString());
	}
}

void ContentBlockingDialog::updateActions()
{
	const QModelIndex index(m_ui->profilesViewWidget->getIndex(m_ui->profilesViewWidget->getCurrentRow()));

	m_ui->updateButton->setEnabled(index.isValid() && index.data(Qt::UserRole + 1).toUrl().isValid());
}

void ContentBlockingDialog::save()
{
	QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	QStringList profiles;

	for (int i = 0; i < m_ui->profilesViewWidget->getRowCount(); ++i)
	{
		profilesSettings.beginGroup(m_ui->profilesViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());
		profilesSettings.setValue(QLatin1String("updateInterval"), m_ui->profilesViewWidget->getIndex(i, 1).data(Qt::DisplayRole));
		profilesSettings.endGroup();

		if (m_ui->profilesViewWidget->getIndex(i, 0).data(Qt::CheckStateRole).toBool())
		{
			profiles.append(m_ui->profilesViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());
		}
	}

	SettingsManager::setValue(QLatin1String("Content/BlockingProfiles"), profiles);

	close();
}

}
