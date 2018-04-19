/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

StartupDialog::StartupDialog(const QString &sessionName, QWidget *parent) : Dialog(parent),
	m_windowsModel(new QStandardItemModel(this)),
	m_ui(new Ui::StartupDialog)
{
	m_ui->setupUi(this);
	m_ui->windowsTreeView->setModel(m_windowsModel);
	m_ui->homePageButton->setEnabled(!SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString().isEmpty());

	const QString pluginsPolicy(SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption).toString());

	if (pluginsPolicy == QLatin1String("enabled"))
	{
		m_ui->enablePluginsCheckBox->setCheckState(Qt::Checked);
	}
	else if (pluginsPolicy == QLatin1String("onDemand"))
	{
		m_ui->enablePluginsCheckBox->setCheckState(Qt::PartiallyChecked);
	}

	const QStringList sessionNames(SessionsManager::getSessions());
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessionNames.count(); ++i)
	{
		const SessionInformation session(SessionsManager::getSession(sessionNames.at(i)));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sessions(information.values());

	for (int i = 0; i < sessions.count(); ++i)
	{
		m_ui->sessionComboBox->addItem((sessions.at(i).title.isEmpty() ? tr("(Untitled)") : sessions.at(i).title), sessions.at(i).path);
	}

	const int index(qMax(0, m_ui->sessionComboBox->findData(sessionName)));

	m_ui->sessionComboBox->setCurrentIndex(index);

	setSession(index);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, [&]()
	{
		switch (m_ui->enablePluginsCheckBox->checkState())
		{
			case Qt::Checked:
				SettingsManager::setOption(SettingsManager::Permissions_EnablePluginsOption, QLatin1String("enabled"));

				break;
			case Qt::PartiallyChecked:
				SettingsManager::setOption(SettingsManager::Permissions_EnablePluginsOption, QLatin1String("onDemand"));

				break;
			default:
				SettingsManager::setOption(SettingsManager::Permissions_EnablePluginsOption, QLatin1String("disabled"));

				break;
		}
	});
	connect(m_ui->buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [&]()
	{
		m_ui->continueSessionWidget->setEnabled(m_ui->continueSessionButton->isChecked());
	});
	connect(m_ui->sessionComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &StartupDialog::setSession);
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

void StartupDialog::setSession(int index)
{
	m_windowsModel->clear();

	const SessionInformation session(SessionsManager::getSession(m_ui->sessionComboBox->itemData(index).toString()));
	QModelIndex activeIndex;
	QFont font(m_ui->windowsTreeView->font());
	font.setBold(true);

	for (int i = 0; i < session.windows.count(); ++i)
	{
		QStandardItem *windowItem(new QStandardItem(tr("Window %1").arg(i + 1)));
		windowItem->setData(session.windows.at(i).geometry, Qt::UserRole);

		m_windowsModel->invisibleRootItem()->appendRow(windowItem);

		for (int j = 0; j < session.windows.at(i).windows.count(); ++j)
		{
			QStandardItem *tabItem(new QStandardItem(session.windows.at(i).windows.at(j).getTitle()));
			tabItem->setFlags(windowItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
			tabItem->setData(Qt::Checked, Qt::CheckStateRole);
			tabItem->setData(tr("Title: %1\nAddress: %2").arg(tabItem->text()).arg(session.windows.at(i).windows.at(j).getUrl()), Qt::ToolTipRole);

			windowItem->appendRow(tabItem);

			if (j == session.windows.at(i).index)
			{
				tabItem->setData(font, Qt::FontRole);

				activeIndex = tabItem->index();
			}
		}

		if (session.windows.count() > 1)
		{
			windowItem->setFlags(windowItem->flags() | Qt::ItemIsUserCheckable);
			windowItem->setData(Qt::Checked, Qt::CheckStateRole);
		}
	}

	m_ui->windowsTreeView->expandAll();
	m_ui->windowsTreeView->scrollTo(activeIndex);
}

SessionInformation StartupDialog::getSession() const
{
	const SessionInformation originalSession(SessionsManager::getSession(m_ui->sessionComboBox->currentData().toString()));
	SessionInformation session;

	if (m_ui->continueSessionButton->isChecked())
	{
		session = originalSession;

		for (int i = (m_windowsModel->rowCount() - 1); i >= 0; --i)
		{
			const QModelIndex mainWindowIndex(m_windowsModel->index(i, 0));
			const int amount(m_windowsModel->rowCount(mainWindowIndex));

			if (!mainWindowIndex.isValid() || i >= session.windows.count() || amount < 1)
			{
				continue;
			}

			if (mainWindowIndex.flags().testFlag(Qt::ItemIsUserCheckable) && mainWindowIndex.data(Qt::CheckStateRole).toInt() == Qt::Unchecked)
			{
				session.windows.removeAt(i);

				continue;
			}

			for (int j = (amount - 1); j >= 0; --j)
			{
				const QModelIndex windowIndex(mainWindowIndex.child(j, 0));

				if (windowIndex.isValid() && windowIndex.flags().testFlag(Qt::ItemIsUserCheckable) && windowIndex.data(Qt::CheckStateRole).toInt() == Qt::Unchecked && j < session.windows.at(i).windows.count())
				{
					session.windows[i].windows.removeAt(j);
				}
			}
		}
	}
	else
	{
		WindowHistoryEntry entry;

		if (m_ui->homePageButton->isChecked())
		{
			entry.url = SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString();
		}
		else if (m_ui->startPageRadioButton->isChecked())
		{
			entry.url = QLatin1String("about:start");
		}
		else
		{
			entry.url = QLatin1String("about:blank");
		}

		SessionWindow window;
		window.history.append(entry);
		window.historyIndex = 0;

		SessionMainWindow mainWindow;
		mainWindow.windows.append(window);

		if (!originalSession.windows.isEmpty())
		{
			const SessionMainWindow originalMainWindow(originalSession.windows.first());

			mainWindow.toolBars = originalMainWindow.toolBars;
			mainWindow.hasToolBarsState = originalMainWindow.hasToolBarsState;
		}

		session.path = QLatin1String("default");
		session.title = tr("Default");
		session.windows.append(mainWindow);
		session.index = 0;
	}

	return session;
}

}
