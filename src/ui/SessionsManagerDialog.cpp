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

#include "SessionsManagerDialog.h"
#include "MainWindow.h"
#include "../core/SessionsManager.h"

#include "ui_SessionsManagerDialog.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTableWidgetItem>

namespace Otter
{

SessionsManagerDialog::SessionsManagerDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::SessionsManagerDialog)
{
	m_ui->setupUi(this);

	const QStringList sessions = SessionsManager::getSessions();
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session = SessionsManager::getSession(sessions.at(i));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted = information.values();
	const QString currentSession = SessionsManager::getCurrentSession();
	int index = 0;

	m_ui->sessionsWidget->setRowCount(sorted.count());

	for (int i = 0; i < sorted.count(); ++i)
	{
		int windows = 0;

		for (int j = 0; j < sorted.at(i).windows.count(); ++j)
		{
			windows += sorted.at(i).windows.at(j).windows.count();
		}

		if (sorted.at(i).path == currentSession)
		{
			index = i;
		}

		m_ui->sessionsWidget->setItem(i, 0, new QTableWidgetItem(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : sorted.at(i).title));
		m_ui->sessionsWidget->setItem(i, 1, new QTableWidgetItem(sorted.at(i).path));
		m_ui->sessionsWidget->setItem(i, 2, new QTableWidgetItem(QStringLiteral("%1 (%2)").arg(sorted.at(i).windows.count()).arg(windows)));
	}

	connect(m_ui->openButton, SIGNAL(clicked()), this, SLOT(openSession()));
	connect(m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteSession()));
	connect(m_ui->sessionsWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentChanged(int)));

	m_ui->sessionsWidget->setCurrentCell(index, 0);
}

SessionsManagerDialog::~SessionsManagerDialog()
{
	delete m_ui;
}

void SessionsManagerDialog::changeEvent(QEvent *event)
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

void SessionsManagerDialog::openSession()
{
	SessionsManager::restoreSession(SessionsManager::getSession(m_ui->sessionsWidget->item(m_ui->sessionsWidget->currentRow(), 1)->data(Qt::DisplayRole).toString()), (m_ui->reuseCheckBox->isChecked() ? qobject_cast<MainWindow*>(parentWidget()) : NULL));

	close();
}

void SessionsManagerDialog::deleteSession()
{
	if (!m_ui->deleteButton->isEnabled())
	{
		return;
	}

	const int index = m_ui->sessionsWidget->currentRow();

	if (QMessageBox::question(this, tr("Confirm"), tr("Are you sure that you want to delete session %1?").arg(m_ui->sessionsWidget->item(index, 0)->data(Qt::DisplayRole).toString()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		if (SessionsManager::deleteSession(m_ui->sessionsWidget->item(index, 1)->data(Qt::DisplayRole).toString()))
		{
			m_ui->sessionsWidget->removeRow(index);
		}
		else
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to delete session."), QMessageBox::Close);
		}
	}
}

void SessionsManagerDialog::currentChanged(int index)
{
	m_ui->deleteButton->setEnabled(m_ui->sessionsWidget->item(index, 1)->data(Qt::DisplayRole).toString() != QLatin1String("default"));
}

}
