/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/Application.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"

#include <QtCore/QOperatingSystemVersion>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOption>

#include <windows.h>

namespace Otter
{

WindowsPlatformStyle::WindowsPlatformStyle(const QString &name) : Style(name),
	m_isModernStyle(QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
{
	checkForModernStyle();

	connect(Application::getInstance(), &Application::paletteChanged, this, &WindowsPlatformStyle::checkForModernStyle);
	connect(ThemesManager::getInstance(), &ThemesManager::widgetStyleChanged, this, &WindowsPlatformStyle::checkForModernStyle);
}

void WindowsPlatformStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element)
	{
		case CE_MenuBarEmptyArea:
		case CE_MenuBarItem:
			if (!m_isModernStyle)
			{
				break;
			}
		case CE_ToolBar:
			{
				bool shouldUseWindowBackground(true);

				if (element == CE_ToolBar)
				{
					const QStyleOptionToolBar *toolBarOption(qstyleoption_cast<const QStyleOptionToolBar*>(option));
					const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));
					const bool isAddressBar(toolBar && toolBar->getIdentifier() == ToolBarsManager::AddressBar);

					if (toolBarOption && toolBar && !(toolBar->getIdentifier() == ToolBarsManager::TabBar && toolBar->getDefinition().location == Qt::TopToolBarArea))
					{
						const ToolBarsManager::ToolBarDefinition tabBarDefinition(ToolBarsManager::getToolBarDefinition(ToolBarsManager::TabBar));

						if ((isAddressBar || toolBarOption->toolBarArea == Qt::TopToolBarArea) && tabBarDefinition.location == Qt::TopToolBarArea)
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

								if (isAddressBar && hasVisibleTabBar)
								{
									shouldUseWindowBackground = false;
								}
							}
						}
						else if (!isAddressBar && toolBarOption->toolBarArea != Qt::TopToolBarArea)
						{
							shouldUseWindowBackground = false;
						}
					}
				}

				const QRect rectantagle((widget && element == CE_ToolBar) ? widget->rect() : option->rect);

				if (shouldUseWindowBackground)
				{
					if (!m_isModernStyle)
					{
						break;
					}

					const DWORD rawColor(GetSysColor(COLOR_WINDOW));

					painter->fillRect(rectantagle, QColor((rawColor & 0xff), ((rawColor >> 8) & 0xff), ((rawColor >> 16) & 0xff)));
				}
				else
				{
					painter->fillRect(rectantagle, Qt::white);
				}

				if (element == CE_MenuBarItem)
				{
					QStyleOptionMenuItem menuItemOption(*qstyleoption_cast<const QStyleOptionMenuItem*>(option));
					menuItemOption.palette.setColor(QPalette::Window, Qt::transparent);

					Style::drawControl(element, &menuItemOption, painter, widget);
				}
				else if (element == CE_ToolBar)
				{
					drawToolBarEdge(option, painter);
				}
			}

			return;
		default:
			break;
	}

	Style::drawControl(element, option, painter, widget);

	if (element == CE_ToolBar)
	{
		drawToolBarEdge(option, painter);
	}
}

void WindowsPlatformStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (m_isModernStyle)
	{
		switch (element)
		{
			case PE_IndicatorTabClose:
				{
					QRect rectangle(option->rect);

					if (rectangle.width() > 10)
					{
						const int offset((10 - rectangle.width()) / 2);

						rectangle = rectangle.marginsAdded(QMargins(offset, offset, offset, offset));
					}

					QPen pen(option->state.testFlag(QStyle::State_Raised) ? QColor(252, 58, 58) : QColor(106, 106, 106));
					pen.setWidthF(1.25);

					painter->save();
					painter->setRenderHint(QPainter::Antialiasing);
					painter->setPen(pen);
					painter->drawLine(rectangle.topLeft(), rectangle.bottomRight());
					painter->drawLine(rectangle.topRight(), rectangle.bottomLeft());
					painter->restore();
				}

				return;
			case PE_PanelStatusBar:
				painter->save();
				painter->fillRect(option->rect, Qt::white);
				painter->setPen(QPen(Qt::lightGray, 1));
				painter->drawLine(option->rect.left(), option->rect.top(), option->rect.right(), option->rect.top());
				painter->restore();

				return;
			default:
				break;
		}
	}

	Style::drawPrimitive(element, option, painter, widget);
}

void WindowsPlatformStyle::checkForModernStyle()
{
	if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
	{
		HIGHCONTRAST information;
		information.cbSize = sizeof(HIGHCONTRAST);

		BOOL isSuccess(SystemParametersInfoW(SPI_GETHIGHCONTRAST, 0, &information, 0));

		m_isModernStyle = !(isSuccess && information.dwFlags & HCF_HIGHCONTRASTON);
	}
	else
	{
		m_isModernStyle = false;
	}
}

int WindowsPlatformStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	if (metric == PM_ToolBarItemSpacing)
	{
		return 3;
	}

	return Style::pixelMetric(metric, option, widget);
}

int WindowsPlatformStyle::getExtraStyleHint(Style::ExtraStyleHint hint) const
{
	if (hint == CanAlignTabBarLabelHint)
	{
		return true;
	}

	return Style::getExtraStyleHint(hint);
}

}
