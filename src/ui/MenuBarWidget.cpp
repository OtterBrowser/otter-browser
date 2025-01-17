/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MenuBarWidget.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ToolBarWidget.h"
#include "../core/SessionsManager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionMenuItem>

namespace Otter
{

MenuBarWidget::MenuBarWidget(MainWindow *parent) : QMenuBar(parent),
	m_mainWindow(parent),
	m_leftToolBar(nullptr),
	m_rightToolBar(nullptr)
{
	QFile file(SessionsManager::getReadableDataPath(QLatin1String("menu/menuBar.json")));
	file.open(QIODevice::ReadOnly);

	const QJsonArray definition(QJsonDocument::fromJson(file.readAll()).array());

	file.close();

	for (int i = 0; i < definition.count(); ++i)
	{
		const QJsonObject object(definition.at(i).toObject());
		Menu *menu(new Menu(Menu::getMenuRoleIdentifier(object.value(QLatin1String("identifier")).toString()), this));
		menu->load(object);

		addMenu(menu);
	}

	if (!isNativeMenuBar())
	{
		reload();

		connect(ToolBarsManager::getInstance(), &ToolBarsManager::toolBarModified, this, [&](int identifier)
		{
			if (identifier == ToolBarsManager::MenuBar)
			{
				reload();
			}
		});
	}
}

void MenuBarWidget::changeEvent(QEvent *event)
{
	QMenuBar::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		QTimer::singleShot(100, this, &MenuBarWidget::updateGeometries);
	}
}

void MenuBarWidget::resizeEvent(QResizeEvent *event)
{
	QMenuBar::resizeEvent(event);

	updateGeometries();
}

void MenuBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu(ToolBarWidget::createCustomizationMenu(ToolBarsManager::MenuBar));
	menu->exec(event->globalPos());
	menu->deleteLater();
}

void MenuBarWidget::reload()
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::MenuBar));
	int menuBarPosition(-1);

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		if (definition.entries.at(i).action == QLatin1String("MenuBarWidget"))
		{
			menuBarPosition = i;

			break;
		}
	}

	const bool isMenuBarOnly(menuBarPosition == 0 && definition.entries.count() == 1);
	const bool needsLeftToolbar(!isMenuBarOnly && menuBarPosition != 0);
	const bool needsRightToolbar(!isMenuBarOnly && menuBarPosition != (definition.entries.count() - 1));

	if (needsLeftToolbar && !m_leftToolBar)
	{
		m_leftToolBar = new ToolBarWidget(ToolBarsManager::MenuBar, m_mainWindow->getActiveWindow(), this);

		setCornerWidget(m_leftToolBar, Qt::TopLeftCorner);
	}
	else if (!needsLeftToolbar && m_leftToolBar)
	{
		m_leftToolBar->deleteLater();
		m_leftToolBar = nullptr;

		setCornerWidget(nullptr, Qt::TopLeftCorner);
	}

	if (needsRightToolbar && !m_rightToolBar)
	{
		m_rightToolBar = new ToolBarWidget(ToolBarsManager::MenuBar, m_mainWindow->getActiveWindow(), this);

		setCornerWidget(m_rightToolBar, Qt::TopRightCorner);
	}
	else if (!needsRightToolbar && m_rightToolBar)
	{
		m_rightToolBar->deleteLater();
		m_rightToolBar = nullptr;

		setCornerWidget(nullptr, Qt::TopRightCorner);
	}

	if (isMenuBarOnly)
	{
		setMaximumHeight(QWIDGETSIZE_MAX);
		setMinimumHeight(0);

		return;
	}

	ToolBarsManager::ToolBarDefinition leftDefinition(definition);
	leftDefinition.entries.clear();

	ToolBarsManager::ToolBarDefinition rightDefinition(definition);
	rightDefinition.entries.clear();

	for (int i = 0; i < definition.entries.count(); ++i)
	{
		if (i != menuBarPosition)
		{
			const ToolBarsManager::ToolBarDefinition::Entry entry(definition.entries.at(i));

			if (i < menuBarPosition)
			{
				leftDefinition.entries.append(entry);
			}
			else
			{
				rightDefinition.entries.append(entry);
			}
		}
	}

	const int menuBarHeight(actionGeometry(this->actions().at(0)).height());
	int toolBarHeight(0);

	if (m_leftToolBar)
	{
		m_leftToolBar->setDefinition(leftDefinition);

		toolBarHeight = (m_leftToolBar->getIconSize() + 12);
	}

	if (m_rightToolBar)
	{
		m_rightToolBar->setDefinition(rightDefinition);

		toolBarHeight = (m_rightToolBar->getIconSize() + 12);
	}

	setFixedHeight((toolBarHeight > menuBarHeight) ? toolBarHeight : menuBarHeight);

	QTimer::singleShot(100, this, &MenuBarWidget::updateGeometries);
}

void MenuBarWidget::updateGeometries()
{
	if (!m_rightToolBar)
	{
		return;
	}

	const int actionsCount(actions().count());
	int size(0);

	if (actionsCount > 0)
	{
		size = ((style()->pixelMetric(QStyle::PM_MenuBarHMargin, nullptr, this) * 2) + (style()->pixelMetric(QStyle::PM_MenuBarItemSpacing, nullptr, this) * actionsCount));

		for (int i = 0; i < actionsCount; ++i)
		{
			QStyleOptionMenuItem option;

			initStyleOption(&option, actions().at(i));

			size += style()->sizeFromContents(QStyle::CT_MenuBarItem, &option, fontMetrics().size(Qt::TextShowMnemonic, option.text), this).width();
		}
	}

	if (width() > (size + (m_leftToolBar ? m_leftToolBar->sizeHint().width() : 0) + m_rightToolBar->sizeHint().width()))
	{
		const int offset(size - (m_leftToolBar ? m_leftToolBar->sizeHint().width() : 0));

		m_rightToolBar->move(offset, 0);
		m_rightToolBar->resize((width() - offset), m_rightToolBar->height());
	}
}

}
