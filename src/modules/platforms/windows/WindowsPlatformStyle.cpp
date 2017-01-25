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

#include <QtCore/QSysInfo>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

#include <windows.h>

namespace Otter
{

WindowsPlatformStyle::WindowsPlatformStyle(const QString &name) : Style(name),
	m_isVistaStyle(QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
{
}

void WindowsPlatformStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (m_isVistaStyle)
	{
		switch (element)
		{
			case QStyle::CE_MenuBarEmptyArea:
			case QStyle::CE_MenuBarItem:
			case QStyle::CE_ToolBar:
				{
					const DWORD rawColor(GetSysColor(COLOR_WINDOW));

					painter->fillRect(option->rect, QColor((rawColor & 0xff), ((rawColor >> 8) & 0xff), ((rawColor >> 16) & 0xff)));

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
	if (m_isVistaStyle && element == QStyle::PE_PanelStatusBar)
	{
		painter->fillRect(option->rect, Qt::white);

		return;
	}

	Style::drawPrimitive(element, option, painter, widget);
}

}
