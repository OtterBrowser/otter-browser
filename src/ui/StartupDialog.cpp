/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StartupDialog.h"
#include "../core/SessionsManager.h"
#include "../core/SettingsManager.h"

#include "ui_StartupDialog.h"

namespace Otter
{

StartupDialog::StartupDialog(const QString &session, QWidget *parent) : Dialog(parent),
	m_windowsModel(new QStandardItemModel(this)),
	m_ui(new Ui::StartupDialog)
{
	m_ui->setupUi(this);
	m_ui->windowsTreeView->setModel(m_windowsModel);

	const QStringList sessions(SessionsManager::getSessions());
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session(SessionsManager::getSession(sessions.at(i)));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted(information.values());

	for (int i = 0; i < sorted.count(); ++i)
	{
		m_ui->sessionComboBox->addItem((sorted.at(i).title.isEmpty() ? tr("(Untitled)") : sorted.at(i).title), sorted.at(i).path);
	}

	const int index(qMax(0, m_ui->sessionComboBox->findData(session)));

	m_ui->sessionComboBox->setCurrentIndex(index);

	setSession(index);

	connect(m_ui->buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(modeChanged()));
	connect(m_ui->sessionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setSession(int)));
}

StartupDialog::~StartupDialog()
{
	delete m_ui;
}

void StartupDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void StartupDialog::modeChanged()
{
	m_ui->continueSessionWidget->setEnabled(m_ui->continueSessionButton->isChecked());
}

void StartupDialog::setSession(int index)
{
	m_windowsModel->clear();

	const SessionInformation session(SessionsManager::getSession(m_ui->sessionComboBox->itemData(index).toString()));
	QFont font(m_ui->windowsTreeView->font());
	font.setBold(true);

	for (int i = 0; i < session.windows.count(); ++i)
	{
		QStandardItem *windowItem(new QStandardItem(tr("Window %1").arg(i + 1)));
		windowItem->setData(session.windows.at(i).geometry, Qt::UserRole);

		for (int j = 0; j < session.windows.at(i).windows.count(); ++j)
		{
			QStandardItem *tabItem(new QStandardItem(session.windows.at(i).windows.at(j).getTitle()));
			tabItem->setFlags(windowItem->flags() | Qt::ItemIsUserCheckable);
			tabItem->setData(Qt::Checked, Qt::CheckStateRole);
			tabItem->setData(tr("Title: %1\nAddress: %2").arg(tabItem->text()).arg(session.windows.at(i).windows.at(j).getUrl()), Qt::ToolTipRole);

			if (j == session.windows.at(i).index)
			{
				tabItem->setData(font, Qt::FontRole);
			}

			windowItem->appendRow(tabItem);
		}

		if (session.windows.count() > 1)
		{
			windowItem->setFlags(windowItem->flags() | Qt::ItemIsUserCheckable);
			windowItem->setData(Qt::Checked, Qt::CheckStateRole);
		}

		m_windowsModel->invisibleRootItem()->appendRow(windowItem);
	}

	m_ui->windowsTreeView->expandAll();
}

SessionInformation StartupDialog::getSession() const
{
	SessionInformation session;

	if (m_ui->continueSessionButton->isChecked())
	{
		QList<SessionMainWindow> windows;

		session = SessionsManager::getSession(m_ui->sessionComboBox->currentData().toString());

		for (int i = 0; i < m_windowsModel->rowCount(); ++i)
		{
			QStandardItem *windowItem(m_windowsModel->item(i, 0));

			if (!windowItem || (windowItem->flags().testFlag(Qt::ItemIsUserCheckable) && windowItem->data(Qt::CheckStateRole).toInt() == Qt::Unchecked))
			{
				continue;
			}

			const int index(session.windows.value(i, SessionMainWindow()).index - 1);
			SessionMainWindow window;
			window.index = (index + 1);
			window.geometry = windowItem->data(Qt::UserRole).toByteArray();

			for (int j = 0; j < windowItem->rowCount(); ++j)
			{
				QStandardItem *tabItem(windowItem->child(j, 0));

				if (tabItem && tabItem->data(Qt::CheckStateRole).toInt() == Qt::Checked)
				{
					window.windows.append(session.windows.value(i, SessionMainWindow()).windows.value(j, SessionWindow()));
				}
				else
				{
					if (j == index)
					{
						window.index = 1;
					}
					else if (j > 0 && j < index)
					{
						--window.index;
					}
				}
			}

			windows.append(window);
		}

		session.windows = windows;
	}
	else
	{
		WindowHistoryEntry entry;

		if (m_ui->homePageButton->isChecked())
		{
			entry.url = SettingsManager::getValue(SettingsManager::Browser_HomePageOption).toString();
		}
		else if (m_ui->startPageRadioButton->isChecked())
		{
			entry.url = QLatin1String("about:start");
		}
		else
		{
			entry.url = QLatin1String("about:blank");
		}

		SessionWindow tab;
		tab.history.append(entry);
		tab.historyIndex = 0;

		SessionMainWindow window;
		window.windows.append(tab);

		session.path = QLatin1String("default");
		session.title = tr("Default");
		session.windows.append(window);
		session.index = 0;
	}

	return session;
}

}
