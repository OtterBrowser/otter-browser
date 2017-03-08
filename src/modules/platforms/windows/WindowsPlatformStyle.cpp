/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WindowsPlatformStyle.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"

#include <QtCore/QSysInfo>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOption>

#include <Windows.h>

namespace Otter
{

WindowsPlatformStyle::WindowsPlatformStyle(const QString &name) : Style(name),
	m_isModernStyle(QSysInfo::windowsVersion() >= QSysInfo::WV_10_0)
{
	checkForVistaStyle();

	connect(QApplication::instance(), SIGNAL(paletteChanged(QPalette)), this, SLOT(checkForVistaStyle()));
	connect(ThemesManager::getInstance(), SIGNAL(widgetStyleChanged()), this, SLOT(checkForVistaStyle()));
}

void WindowsPlatformStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (m_isModernStyle)
	{
		switch (element)
		{
			case QStyle::CE_MenuBarEmptyArea:
			case QStyle::CE_MenuBarItem:
			case QStyle::CE_ToolBar:
				{
					bool shouldUseWindowBackground(true);

					if (element == QStyle::CE_ToolBar)
					{
						const QStyleOptionToolBar *toolBarOption(qstyleoption_cast<const QStyleOptionToolBar*>(option));
						const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));
						const bool isNavigationBar(toolBar && toolBar->getIdentifier() == ToolBarsManager::NavigationBar);

						if (toolBarOption && toolBar && !(toolBar->getIdentifier() == ToolBarsManager::TabBar && toolBar->getDefinition().location == Qt::TopToolBarArea))
						{
							const ToolBarsManager::ToolBarDefinition tabBarDefinition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar));

							if ((isNavigationBar || toolBarOption->toolBarArea == Qt::TopToolBarArea) && tabBarDefinition.location == Qt::TopToolBarArea)
							{
								MainWindow *mainWindow(MainWindow::findMainWindow(widget->parentWidget()));

								if (mainWindow)
								{
									const QVector<ToolBarWidget*> toolBars(mainWindow->getToolBars(Qt::TopToolBarArea));
									bool hasVisibleTabBar(false);

									for (int i = 0; i < toolBars.count(); ++i)
									{
										if (toolBars.at(i)->getIdentifier() == ToolBarsManager::TabBar && toolBars.at(i)->isVisible())
										{
											hasVisibleTabBar = true;

											break;
										}

										if (toolBars.at(i) == toolBar && !hasVisibleTabBar)
										{
											shouldUseWindowBackground = false;

											break;
										}
									}

									if (isNavigationBar && hasVisibleTabBar)
									{
										shouldUseWindowBackground = false;
									}
								}
							}
							else if (!isNavigationBar && toolBarOption->toolBarArea != Qt::TopToolBarArea)
							{
								shouldUseWindowBackground = false;
							}
						}
					}

					const QRect rectantagle((widget && element == QStyle::CE_ToolBar) ? widget->rect() : option->rect);

					if (shouldUseWindowBackground)
					{
						const DWORD rawColor(GetSysColor(COLOR_WINDOW));

						painter->fillRect(rectantagle, QColor((rawColor & 0xff), ((rawColor >> 8) & 0xff), ((rawColor >> 16) & 0xff)));
					}
					else
					{
						painter->fillRect(rectantagle, Qt::white);
					}

					if (element == QStyle::CE_MenuBarItem)
					{
						QStyleOptionMenuItem menuItemOption(*qstyleoption_cast<const QStyleOptionMenuItem*>(option));
						menuItemOption.palette.setColor(QPalette::Window, Qt::transparent);

						Style::drawControl(element, &menuItemOption, painter, widget);
					}
				}

				return;
			default:
				break;
		}
	}

	Style::drawControl(element, option, painter, widget);
}

void WindowsPlatformStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (m_isModernStyle && element == QStyle::PE_PanelStatusBar)
	{
		painter->fillRect(option->rect, Qt::white);

		return;
	}

	Style::drawPrimitive(element, option, painter, widget);
}

void WindowsPlatformStyle::checkForVistaStyle()
{
	if (QSysInfo::windowsVersion() >= QSysInfo::WV_10_0)
	{
		HIGHCONTRAST information = {0};
		information.cbSize = sizeof(HIGHCONTRAST);

		BOOL isSuccess(SystemParametersInfoW(SPI_GETHIGHCONTRAST, 0, &information, 0));

		m_isModernStyle = !(isSuccess && information.dwFlags & HCF_HIGHCONTRASTON);
	}
	else
	{
		m_isModernStyle = false;
	}
}

}
