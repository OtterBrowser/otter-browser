/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "UserAgentsManagerDialog.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/ThemesManager.h"
#include "../core/Utils.h"

#include "ui_UserAgentsManagerDialog.h"

namespace Otter
{

UserAgentsManagerDialog::UserAgentsManagerDialog(QList<UserAgentDefinition> userAgents, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::UserAgentsManagerDialog)
{
	m_ui->setupUi(this);
	m_ui->moveDownButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-down")));
	m_ui->moveUpButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-up")));

	QStandardItemModel *model(new QStandardItemModel(this));
	model->setHorizontalHeaderLabels(QStringList({tr("Title"), tr("Value")}));

	for (int i = 0; i < userAgents.count(); ++i)
	{
		QList<QStandardItem*> items({new QStandardItem(userAgents.at(i).getTitle()), new QStandardItem(userAgents.at(i).value)});
		items[0]->setData(userAgents.at(i).identifier, Qt::UserRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);

		model->appendRow(items);
	}

	m_ui->userAgentsView->setModel(model);

	connect(m_ui->userAgentsView, SIGNAL(canMoveDownChanged(bool)), m_ui->moveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->userAgentsView, SIGNAL(canMoveUpChanged(bool)), m_ui->moveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->userAgentsView, SIGNAL(needsActionsUpdate()), this, SLOT(updateUserAgentActions()));
	connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addUserAgent()));
	connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(removeUserAgent()));
	connect(m_ui->moveDownButton, SIGNAL(clicked()), m_ui->userAgentsView, SLOT(moveDownRow()));
	connect(m_ui->moveUpButton, SIGNAL(clicked()), m_ui->userAgentsView, SLOT(moveUpRow()));
}

UserAgentsManagerDialog::~UserAgentsManagerDialog()
{
	delete m_ui;
}

void UserAgentsManagerDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void UserAgentsManagerDialog::addUserAgent()
{
	QList<QStandardItem*> items({new QStandardItem(), new QStandardItem()});
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);

	m_ui->userAgentsView->insertRow(items);
}

void UserAgentsManagerDialog::removeUserAgent()
{
	m_ui->userAgentsView->removeRow();
}

void UserAgentsManagerDialog::updateUserAgentActions()
{
	const int currentRow(m_ui->userAgentsView->getCurrentRow());

	m_ui->removeButton->setEnabled(currentRow >= 0 && currentRow < m_ui->userAgentsView->getRowCount());
}

QList<UserAgentDefinition> UserAgentsManagerDialog::getUserAgents() const
{
	QList<UserAgentDefinition> userAgents;
	QStringList identifiers;

	for (int i = 0; i < m_ui->userAgentsView->getRowCount(); ++i)
	{
		QString identifier(m_ui->userAgentsView->getIndex(i, 0).data(Qt::UserRole).toString());

		if (identifier.isEmpty() || identifiers.contains(identifier))
		{
			identifier = Utils::createIdentifier(QString(), identifiers);
		}

		identifiers.append(identifier);

		UserAgentDefinition userAgent;
		userAgent.identifier = identifier;
		userAgent.title = m_ui->userAgentsView->getIndex(i, 0).data(Qt::DisplayRole).toString();
		userAgent.value = m_ui->userAgentsView->getIndex(i, 1).data(Qt::DisplayRole).toString();

		userAgents.append(userAgent);
	}

	return userAgents;
}

}
