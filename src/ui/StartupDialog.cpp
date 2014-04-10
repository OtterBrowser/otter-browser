/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StartupDialog.h"
#include "../core/SessionsManager.h"

#include "ui_StartupDialog.h"

namespace Otter
{

StartupDialog::StartupDialog(const QString &session, QWidget *parent) : QDialog(parent),
	m_windowsModel(new QStandardItemModel(this)),
	m_ui(new Ui::StartupDialog)
{
	m_ui->setupUi(this);
	m_ui->windowsTreeView->setModel(m_windowsModel);

	const QStringList sessions = SessionsManager::getSessions();
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session = SessionsManager::getSession(sessions.at(i));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted = information.values();

	for (int i = 0; i < sorted.count(); ++i)
	{
		m_ui->sessionComboBox->addItem((sorted.at(i).title.isEmpty() ? tr("(Untitled)") : sorted.at(i).title), sorted.at(i).path);
	}

	const int index = qMax(0, m_ui->sessionComboBox->findData(session));

	m_ui->sessionComboBox->setCurrentIndex(index);

	setSession(index);

	connect(m_ui->sessionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setSession(int)));
}

StartupDialog::~StartupDialog()
{
	delete m_ui;
}

void StartupDialog::setSession(int index)
{
	m_windowsModel->clear();

	const SessionInformation session = SessionsManager::getSession(m_ui->sessionComboBox->itemData(index).toString());
	QFont font(m_ui->windowsTreeView->font());
	font.setBold(true);

	for (int i = 0; i < session.windows.count(); ++i)
	{
		QStandardItem *windowItem = ((session.windows.count() == 1) ? m_windowsModel->invisibleRootItem() : new QStandardItem(tr("Window %1").arg(i + 1)));

		for (int j = 0; j < session.windows.at(i).windows.count(); ++j)
		{
			QStandardItem *tabItem = new QStandardItem(session.windows.at(i).windows.at(j).getTitle());
			tabItem->setFlags(windowItem->flags() | Qt::ItemIsUserCheckable);
			tabItem->setData(Qt::Checked, Qt::CheckStateRole);

			if (j == session.windows.at(i).index)
			{
				tabItem->setData(font, Qt::FontRole);
			}

			windowItem->appendRow(tabItem);
		}

		if (windowItem != m_windowsModel->invisibleRootItem())
		{
			windowItem->setFlags(windowItem->flags() | Qt::ItemIsUserCheckable);
			windowItem->setData(Qt::Checked, Qt::CheckStateRole);

			m_windowsModel->invisibleRootItem()->appendRow(windowItem);
		}
	}
}

}
