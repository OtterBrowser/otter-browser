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

#include "MacPlatformStyle.h"

#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

namespace Otter
{

MacPlatformStyle::MacPlatformStyle(const QString &name) : Style(name)
{
}

void MacPlatformStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element)
	{
		case CE_TabBarTab:
			{
				const QStyleOptionTab *tabOption(qstyleoption_cast<const QStyleOptionTab*>(option));

				if (tabOption && tabOption->documentMode)
				{
					painter->save();

					if (tabOption->position == QStyleOptionTab::Beginning || tabOption->position == QStyleOptionTab::Middle)
					{
						painter->setPen(QColor(100, 100, 100, 200));
						painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
					}

					if (option->state.testFlag(State_MouseOver))
					{
						painter->fillRect(option->rect, QColor(0, 0, 0, (option->state.testFlag(State_Selected) ? 80 : 50)));
					}

					painter->restore();

					return;
				}
			}

			break;
		case CE_ToolBar:
			QProxyStyle::drawControl(element, option, painter, widget);

			return;
		default:
			break;
	}

	Style::drawControl(element, option, painter, widget);
}

void MacPlatformStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element)
	{
		case PE_IndicatorTabClose:
			if (widget && widget->underMouse())
			{
				QRect rectangle(option->rect);

				if (rectangle.width() > 8)
				{
					const int offset((8 - rectangle.width()) / 2);

					rectangle = rectangle.marginsAdded(QMargins(offset, offset, offset, offset));
				}

				QPen pen(QColor(60, 60, 60));
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
			QProxyStyle::drawPrimitive(element, option, painter, widget);

			return;
		case PE_FrameTabBarBase:
			{
				const QStyleOptionTabBarBase *tabBarBaseOption(qstyleoption_cast<const QStyleOptionTabBarBase*>(option));

				if (tabBarBaseOption && tabBarBaseOption->documentMode)
				{
					painter->save();
					painter->setPen(QColor(100, 100, 100, 200));

					if (tabBarBaseOption->selectedTabRect.isValid())
					{
						QRect leftRectangle(tabBarBaseOption->tabBarRect);
						leftRectangle.setRight(tabBarBaseOption->selectedTabRect.left());
						leftRectangle.setLeft(option->rect.left());

						QRect rightRectangle(tabBarBaseOption->tabBarRect);
						rightRectangle.setLeft(tabBarBaseOption->selectedTabRect.right());
						rightRectangle.setRight(option->rect.right());

						painter->fillRect(leftRectangle, QColor(0, 0, 0, 50));
						painter->fillRect(rightRectangle, QColor(0, 0, 0, 50));
					}
					else
					{
						painter->fillRect(tabBarBaseOption->tabBarRect, QColor(0, 0, 0, 50));
					}

					painter->drawLine(QPoint(option->rect.left(), tabBarBaseOption->tabBarRect.top()), QPoint(option->rect.right(), tabBarBaseOption->tabBarRect.top()));
					painter->drawLine(QPoint(option->rect.left(), tabBarBaseOption->tabBarRect.bottom()), QPoint(option->rect.right(), tabBarBaseOption->tabBarRect.bottom()));

					if (tabBarBaseOption->tabBarRect.left() != option->rect.left())
					{
						painter->drawLine(tabBarBaseOption->tabBarRect.topLeft(), tabBarBaseOption->tabBarRect.bottomLeft());
					}

					if (tabBarBaseOption->tabBarRect.right() != option->rect.right())
					{
						painter->drawLine(tabBarBaseOption->tabBarRect.topRight(), tabBarBaseOption->tabBarRect.bottomRight());
					}

					painter->restore();

					return;
				}
			}

			break;
		default:
			break;
	}

	Style::drawPrimitive(element, option, painter, widget);
}

QString MacPlatformStyle::getStyleSheet() const
{
	return QLatin1String("QToolBar QLineEdit {border:1px solid #AFAFAF;border-radius:4px;}");
}

int MacPlatformStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	if (metric == PM_ToolBarIconSize)
	{
		return 16;
	}

	return Style::pixelMetric(metric, option, widget);
}

}
