/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SessionsManagerDialog.h"
#include "MainWindow.h"
#include "../core/Application.h"
#include "../core/SessionsManager.h"

#include "ui_SessionsManagerDialog.h"

#include <QtWidgets/QMessageBox>

namespace Otter
{

SessionsManagerDialog::SessionsManagerDialog(QWidget *parent) : Dialog(parent),
	m_ui(new Ui::SessionsManagerDialog)
{
	m_ui->setupUi(this);
	m_ui->openInExistingWindowCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Sessions_OpenInExistingWindowOption).toBool());

	const QStringList sessions(SessionsManager::getSessions());
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session(SessionsManager::getSession(sessions.at(i)));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	QStandardItemModel *model(new QStandardItemModel(this));
	model->setHorizontalHeaderLabels({tr("Title"), tr("Identifier"), tr("Windows")});

	const QList<SessionInformation> sorted(information.values());
	const QString currentSession(SessionsManager::getCurrentSession());
	int row(0);

	for (int i = 0; i < sorted.count(); ++i)
	{
		const SessionInformation session(sorted.at(i));
		int windows(0);

		for (int j = 0; j < session.windows.count(); ++j)
		{
			windows += session.windows.at(j).windows.count();
		}

		if (session.path == currentSession)
		{
			row = i;
		}

		QList<QStandardItem*> items({new QStandardItem(session.title.isEmpty() ? tr("(Untitled)") : session.title), new QStandardItem(session.path), new QStandardItem(tr("%n window(s) (%1)", "", session.windows.count()).arg(tr("%n tab(s)", "", windows)))});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		model->appendRow(items);
	}

	m_ui->sessionsViewWidget->setModel(model);

	connect(m_ui->openButton, &QPushButton::clicked, this, &SessionsManagerDialog::openSession);
	connect(m_ui->deleteButton, &QPushButton::clicked, this, &SessionsManagerDialog::deleteSession);
	connect(m_ui->sessionsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &SessionsManagerDialog::updateActions);

	m_ui->sessionsViewWidget->setCurrentIndex(m_ui->sessionsViewWidget->getIndex(row, 0));
}

SessionsManagerDialog::~SessionsManagerDialog()
{
	delete m_ui;
}

void SessionsManagerDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->sessionsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Title"), tr("Identifier"), tr("Windows")});
	}
}

void SessionsManagerDialog::openSession()
{
	const SessionInformation session(SessionsManager::getSession(m_ui->sessionsViewWidget->getIndex(m_ui->sessionsViewWidget->getCurrentRow(), 1).data(Qt::DisplayRole).toString()));

	if (session.isClean || QMessageBox::warning(this, tr("Warning"), tr("This session was not saved correctly.\nAre you sure that you want to restore this session anyway?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		SessionsManager::restoreSession(session, (m_ui->openInExistingWindowCheckBox->isChecked() ? Application::getActiveWindow() : nullptr));

		close();
	}
}

void SessionsManagerDialog::deleteSession()
{
	if (!m_ui->deleteButton->isEnabled())
	{
		return;
	}

	const int row(m_ui->sessionsViewWidget->getCurrentRow());

	if (QMessageBox::question(this, tr("Confirm"), tr("Are you sure that you want to delete session %1?").arg(m_ui->sessionsViewWidget->getIndex(row, 0).data(Qt::DisplayRole).toString()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		if (SessionsManager::deleteSession(m_ui->sessionsViewWidget->getIndex(row, 1).data(Qt::DisplayRole).toString()))
		{
			m_ui->sessionsViewWidget->removeRow();
		}
		else
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to delete session."), QMessageBox::Close);
		}
	}
}

void SessionsManagerDialog::updateActions()
{
	m_ui->deleteButton->setEnabled(m_ui->sessionsViewWidget->getIndex(m_ui->sessionsViewWidget->getCurrentRow(), 1).data(Qt::DisplayRole).toString() != QLatin1String("default"));
}

}
