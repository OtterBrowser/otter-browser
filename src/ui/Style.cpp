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
#include "ItemViewWidget.h"
#include "ToolBarWidget.h"
#include "../core/SettingsManager.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QStyleOption>

namespace Otter
{

Style::Style(const QString &name) : QProxyStyle(name.isEmpty() ? nullptr : QStyleFactory::create(name)),
	m_areToolTipsEnabled(SettingsManager::getOption(SettingsManager::Browser_ToolTipsModeOption).toString() != QLatin1String("disabled")),
	m_canAlignTabBarLabel(false)
{
	const QString styleName(getName());

	if (styleName == QLatin1String("fusion") || styleName == QLatin1String("breeze") || styleName == QLatin1String("oxygen"))
	{
		m_canAlignTabBarLabel = true;
	}

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &Style::handleOptionChanged);
}

void Style::drawDropZone(const QLine &line, QPainter *painter) const
{
	painter->save();
	painter->setPen(QPen(QGuiApplication::palette().highlight(), 3, Qt::DotLine));
	painter->drawLine(line);
	painter->setPen(QPen(QGuiApplication::palette().highlight(), 9, Qt::SolidLine, Qt::RoundCap));
	painter->drawPoint(line.p1());
	painter->drawPoint(line.p2());
	painter->restore();
}

void Style::drawToolBarEdge(const QStyleOption *option, QPainter *painter) const
{
	const QStyleOptionToolBar *toolBarOption(qstyleoption_cast<const QStyleOptionToolBar*>(option));

	if (!toolBarOption || toolBarOption->positionOfLine == QStyleOptionToolBar::Beginning || toolBarOption->positionOfLine == QStyleOptionToolBar::Middle || toolBarOption->toolBarArea == Qt::NoToolBarArea)
	{
		return;
	}

	painter->save();
	painter->setPen(QPen(Qt::lightGray, 1));

	switch (toolBarOption->toolBarArea)
	{
		case Qt::LeftToolBarArea:
			painter->drawLine(option->rect.right(), option->rect.top(), option->rect.right(), option->rect.bottom());

			break;
		case Qt::RightToolBarArea:
			painter->drawLine(option->rect.left(), option->rect.top(), option->rect.left(), option->rect.bottom());

			break;
		case Qt::BottomToolBarArea:
			painter->drawLine(option->rect.left(), option->rect.top(), option->rect.right(), option->rect.top());

			break;
		default:
			painter->drawLine(option->rect.left(), option->rect.bottom(), option->rect.right(), option->rect.bottom());

			break;
	}

	painter->restore();
}

void Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element)
	{
		case CE_HeaderLabel:
			if (option->rect.width() >= 30)
			{
				QProxyStyle::drawControl(element, option, painter, widget);
			}

			break;
		case CE_ToolBar:
			QProxyStyle::drawControl(element, option, painter, widget);

			drawToolBarEdge(option, painter);

			break;
		default:
			QProxyStyle::drawControl(element, option, painter, widget);

			break;
	}
}

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element)
	{
		case PE_IndicatorItemViewItemCheck:
			if (widget)
			{
				const ItemViewWidget *view(qobject_cast<const ItemViewWidget*>(widget));

				if (view && view->isExclusive())
				{
					QStyleOptionButton buttonOption;
					buttonOption.rect = option->rect;
					buttonOption.state = option->state;

					drawControl(CE_RadioButton, &buttonOption, painter);

					return;
				}
			}

			break;
		case QStyle::PE_IndicatorTabClose:
			{
				const int size(pixelMetric(QStyle::PM_TabCloseIndicatorWidth, option, widget));

				if (option->rect.width() < size)
				{
					const qreal scale(option->rect.width() / static_cast<qreal>(size));
					QStyleOption indicatorOption;
					indicatorOption.state = option->state;
					indicatorOption.rect = QRect(0, 0, size, size);

					painter->save();
					painter->setRenderHint(QPainter::SmoothPixmapTransform);
					painter->translate(option->rect.topLeft());
					painter->scale(scale, scale);

					QProxyStyle::drawPrimitive(element, &indicatorOption, painter, widget);

					painter->restore();

					return;
				}
			}

			break;
		case PE_PanelStatusBar:
			QProxyStyle::drawPrimitive(element, option, painter, widget);

			painter->save();
			painter->setPen(QPen(Qt::lightGray, 1));
			painter->drawLine(option->rect.left(), option->rect.top(), option->rect.right(), option->rect.top());
			painter->restore();

			return;
		default:
			break;
	}

	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::handleOptionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Browser_ToolTipsModeOption)
	{
		m_areToolTipsEnabled = (value.toString() != QLatin1String("disabled"));
	}
}

QString Style::getName() const
{
	return (baseStyle() ? baseStyle() : this)->objectName().toLower();
}

QString Style::getStyleSheet() const
{
	return {};
}

QRect Style::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
	switch (element)
	{
		case SE_TabBarTabLeftButton:
			{
				if (qstyleoption_cast<const QStyleOptionTab*>(option))
				{
					return option->rect;
				}

				QStyleOptionTab tabOption;
				tabOption.documentMode = true;
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

				const int offset(QProxyStyle::subElementRect((QGuiApplication::isLeftToRight() ? SE_TabBarTabLeftButton : SE_TabBarTabRightButton), &tabOption, widget).left() - option->rect.left());
				QRect rectangle(option->rect);
				rectangle.setLeft(rectangle.left() + offset);
				rectangle.setRight(rectangle.right() - offset);

				return rectangle;
			}
		case SE_TabBarTabText:
			{
				const QStyleOptionTab *tabOption(qstyleoption_cast<const QStyleOptionTab*>(option));

				if (tabOption && tabOption->documentMode)
				{
					return option->rect;
				}
			}

			break;
		case SE_ToolBarHandle:
			if (widget)
			{
				const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));

				if (toolBar && toolBar->getIdentifier() == ToolBarsManager::TabBar && toolBar->isMovable())
				{
					const int offset(QProxyStyle::pixelMetric(PM_ToolBarItemMargin, option, widget) + QProxyStyle::pixelMetric(PM_ToolBarFrameWidth, option, widget));
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

			break;
		default:
			break;
	}

	return QProxyStyle::subElementRect(element, option, widget);
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	switch (metric)
	{
		case PM_TabBarTabHSpace:
			if (QProxyStyle::pixelMetric(metric, option, widget) == QCommonStyle::pixelMetric(metric, option, widget))
			{
				const QStyleOptionTab *tabOption(qstyleoption_cast<const QStyleOptionTab*>(option));

				if (tabOption && tabOption->documentMode)
				{
					return QProxyStyle::pixelMetric(PM_TabBarTabVSpace, option, widget);
				}
			}

			break;
		case PM_ToolBarFrameWidth:
		case PM_ToolBarHandleExtent:
		case PM_ToolBarItemMargin:
			if (widget)
			{
				const ToolBarWidget *toolBar(qobject_cast<const ToolBarWidget*>(widget));

				if (toolBar && toolBar->getIdentifier() == ToolBarsManager::TabBar)
				{
					if (metric == PM_ToolBarHandleExtent)
					{
						return (QProxyStyle::pixelMetric(metric, option, widget) + QProxyStyle::pixelMetric(PM_ToolBarItemMargin, option, widget) + QProxyStyle::pixelMetric(PM_ToolBarFrameWidth, option, widget));
					}

					return 0;
				}
			}
		default:
			break;
	}

	return QProxyStyle::pixelMetric(metric, option, widget);
}

int Style::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
	if (!m_areToolTipsEnabled)
	{
		switch (hint)
		{
			case SH_ToolTip_FallAsleepDelay:
			case SH_ToolTip_WakeUpDelay:
				return std::numeric_limits<int>::max();
			default:
				break;
		}
	}

	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

int Style::getExtraStyleHint(Style::ExtraStyleHint hint) const
{
	if (hint == CanAlignTabBarLabelHint)
	{
		return (m_canAlignTabBarLabel ? 1 : 0);
	}

	return 0;
}

}
