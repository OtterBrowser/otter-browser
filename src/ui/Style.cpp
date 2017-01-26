/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Style.h"
#include "ToolBarWidget.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QStyleOption>

namespace Otter
{

Style::Style(const QString &name) : QProxyStyle(name.isEmpty() ? nullptr : QStyleFactory::create(name))
{
}

void Style::drawDropZone(const QLine &line, QPainter *painter)
{
	painter->save();
	painter->setPen(QPen(QGuiApplication::palette().text(), 3, Qt::DotLine));
	painter->drawLine(line);
	painter->setPen(QPen(QGuiApplication::palette().text(), 9, Qt::SolidLine, Qt::RoundCap));
	painter->drawPoint(line.p1());
	painter->drawPoint(line.p2());
	painter->restore();
}

QString Style::getName() const
{
	return (baseStyle() ? baseStyle() : this)->objectName().toLower();
}

QRect Style::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
	switch (element)
	{
		case QStyle::SE_TabBarTabLeftButton:
			{
				if (qstyleoption_cast<const QStyleOptionTab*>(option))
				{
					return option->rect;
				}

				QStyleOptionTab tabOption;
				tabOption.rect = option->rect;
				tabOption.shape = QTabBar::RoundedNorth;

				if (QGuiApplication::isLeftToRight())
				{
					tabOption.cornerWidgets = QStyleOptionTab::LeftCornerWidget;
					tabOption.leftButtonSize = QSize(1, 1);
				}
				else
				{
					tabOption.cornerWidgets = QStyleOptionTab::RightCornerWidget;
					tabOption.rightButtonSize = QSize(1, 1);
				}

				const int offset(QProxyStyle::subElementRect((QGuiApplication::isLeftToRight() ? QStyle::SE_TabBarTabLeftButton : QStyle::SE_TabBarTabRightButton), &tabOption, widget).left() - option->rect.left());
				QRect rectangle(option->rect);
				rectangle.setLeft(rectangle.left() + offset);
				rectangle.setRight(rectangle.right() - offset);

				return rectangle;
			}
		case QStyle::SE_ToolBarHandle:
			if (widget)
			{
				const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));

				if (toolBar && toolBar->getIdentifier() == ToolBarsManager::TabBar && toolBar->isMovable())
				{
					const int offset(QProxyStyle::pixelMetric(QStyle::PM_ToolBarItemMargin, option, widget) + QProxyStyle::pixelMetric(QStyle::PM_ToolBarFrameWidth, option, widget));
					QRect rectangle(QProxyStyle::subElementRect(element, option, widget));

					if (QGuiApplication::isLeftToRight())
					{
						rectangle.translate(offset, 0);
					}
					else
					{
						rectangle.translate(-offset, 0);
					}

					return rectangle;
				}
			}
		default:
			break;
	}

	return QProxyStyle::subElementRect(element, option, widget);
}

int Style::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	switch (metric)
	{
		case QStyle::PM_TabBarTabHSpace:
			if (QProxyStyle::pixelMetric(metric, option, widget) == QCommonStyle::pixelMetric(metric, option, widget))
			{
				return QProxyStyle::pixelMetric(QStyle::PM_TabBarTabVSpace, option, widget);
			}

			break;
		case QStyle::PM_ToolBarItemMargin:
		case QStyle::PM_ToolBarFrameWidth:
		case QStyle::PM_ToolBarHandleExtent:
			if (widget)
			{
				const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));

				if (toolBar && toolBar->getIdentifier() == ToolBarsManager::TabBar)
				{
					if (QStyle::PM_ToolBarHandleExtent)
					{
						return (QProxyStyle::pixelMetric(metric, option, widget) + QProxyStyle::pixelMetric(QStyle::PM_ToolBarItemMargin, option, widget) + QProxyStyle::pixelMetric(QStyle::PM_ToolBarFrameWidth, option, widget));
					}

					return 0;
				}
			}
		default:
			break;
	}

	return QProxyStyle::pixelMetric(metric, option, widget);
}

}
