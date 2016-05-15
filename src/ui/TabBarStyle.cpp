/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabBarStyle.h"

#include <QtWidgets/QStyleOption>

namespace Otter
{

TabBarStyle::TabBarStyle(QStyle *style) : QProxyStyle(style)
{
}

void TabBarStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (element == CE_TabBarTabLabel)
	{
		const QStyleOptionTab *tabOption(qstyleoption_cast<const QStyleOptionTab*>(option));

		if (tabOption)
		{
			QStyleOptionTab mutableTabOption(*tabOption);
			mutableTabOption.shape = QTabBar::RoundedNorth;

			QProxyStyle::drawControl(element, &mutableTabOption, painter, widget);

			return;
		}
	}

	QProxyStyle::drawControl(element, option, painter, widget);
}

QSize TabBarStyle::sizeFromContents(QStyle::ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
	QSize mutableSize(QProxyStyle::sizeFromContents(type, option, size, widget));
	const QStyleOptionTab *tabOption(qstyleoption_cast<const QStyleOptionTab*>(option));

	if (type == QStyle::CT_TabBarTab && tabOption && (tabOption->shape == QTabBar::RoundedEast || tabOption->shape == QTabBar::RoundedWest))
	{
		mutableSize.transpose();
	}

	return mutableSize;
}

QRect TabBarStyle::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
	if (element == QStyle::SE_TabBarTabLeftButton || element == QStyle::SE_TabBarTabRightButton || element == QStyle::SE_TabBarTabText)
	{
		const QStyleOptionTab *tabOption(qstyleoption_cast<const QStyleOptionTab*>(option));

		if (tabOption->shape == QTabBar::RoundedEast || tabOption->shape == QTabBar::RoundedWest)
		{
			QStyleOptionTab mutableTabOption(*tabOption);
			mutableTabOption.shape = QTabBar::RoundedNorth;

			QRect rectangle(QProxyStyle::subElementRect(element, &mutableTabOption, widget));
			rectangle.translate(0, option->rect.top());

			return rectangle;
		}
	}

	return QProxyStyle::subElementRect(element, option, widget);
}

}
