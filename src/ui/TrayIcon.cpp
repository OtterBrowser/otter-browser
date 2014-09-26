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

#include "TrayIcon.h"
#include "MainWindow.h"
#include "../core/Application.h"
#include "../core/SessionsManager.h"

#include <QtWidgets/QMenu>

namespace Otter
{

TrayIcon::TrayIcon(Application *parent) : QObject(parent),
	m_icon(new QSystemTrayIcon(this)),
	m_isHidden(false)
{
	QMenu *menu = new QMenu();
	menu->addAction(tr("Show Windows"))->setData(QLatin1String("toggleVisibility"));
	menu->addSeparator();
	menu->addAction(tr("New Tab"))->setData(QLatin1String("newTab"));
	menu->addAction(tr("New Private Tab"))->setData(QLatin1String("newPrivateTab"));
	menu->addSeparator();
	menu->addAction(tr("Bookmarks"))->setData(QLatin1String("bookmarks"));
	menu->addAction(tr("Transfers"))->setData(QLatin1String("transfers"));
	menu->addAction(tr("History"))->setData(QLatin1String("history"));
	menu->addSeparator();
	menu->addAction(tr("Exit"))->setData(QLatin1String("exit"));

	m_icon->setIcon(parent->windowIcon());
	m_icon->setContextMenu(menu);
	m_icon->setToolTip(tr("Otter Browser"));
	m_icon->show();

	setParent(NULL);

	connect(Application::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(hide()));
	connect(this, SIGNAL(destroyed()), menu, SLOT(deleteLater()));
	connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
	connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(triggerAction(QAction*)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	connect(m_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)));
}

void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
	{
		const QList<MainWindow*> windows = SessionsManager::getWindows();

		for (int i = 0; i < windows.count(); ++i)
		{
			if (!windows.at(i))
			{
				continue;
			}

			if (m_isHidden)
			{
				windows.at(i)->show();
				windows.at(i)->activateWindow();
				windows.at(i)->restoreWindowState();
			}
			else
			{
				windows.at(i)->storeWindowState();
				windows.at(i)->hide();
			}
		}

		m_isHidden = !m_isHidden;
	}
}

void TrayIcon::triggerAction(QAction *action)
{
	if (!action || action->data().isNull())
	{
		return;
	}

	const QString identifier = action->data().toString();

	if (identifier == QLatin1String("toggleVisibility"))
	{
		activated(QSystemTrayIcon::Trigger);
	}
	else if (identifier == QLatin1String("exit"))
	{
		Application::getInstance()->close();
	}
	else
	{
		WindowsManager *manager = SessionsManager::getWindowsManager();

		if (manager)
		{
			if (identifier == QLatin1String("newTab"))
			{
				manager->open(QUrl(), DefaultOpen);
			}
			else if (identifier == QLatin1String("newPrivateTab"))
			{
				manager->open(QUrl(), PrivateOpen);
			}
			else if (identifier == QLatin1String("bookmarks"))
			{
				manager->open(QUrl(QLatin1String("about:bookmarks")));
			}
			else if (identifier == QLatin1String("transfers"))
			{
				manager->open(QUrl(QLatin1String("about:transfers")));
			}
			else if (identifier == QLatin1String("history"))
			{
				manager->open(QUrl(QLatin1String("about:history")));
			}
		}
	}
}

void TrayIcon::hide()
{
	m_icon->hide();

	Application::getInstance()->processEvents();
}

void TrayIcon::updateMenu()
{
	m_icon->contextMenu()->actions().at(0)->setText(m_isHidden ? tr("Show Windows") : tr("Hide Windows"));
}

}
