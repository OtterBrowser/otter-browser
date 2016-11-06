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

#include "StatusBarWidget.h"
#include "MainWindow.h"
#include "ToolBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Otter
{

StatusBarWidget::StatusBarWidget(MainWindow *parent) : QStatusBar(parent),
	m_toolBar(new ToolBarWidget(ToolBarsManager::StatusBar, nullptr, this))
{
	optionChanged(SettingsManager::Interface_ShowSizeGripOption, SettingsManager::getValue(SettingsManager::Interface_ShowSizeGripOption));
	setFixedHeight(ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).iconSize);

	QTimer::singleShot(100, this, SLOT(updateSize()));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(toolBarModified(int)));
}

void StatusBarWidget::changeEvent(QEvent *event)
{
	QStatusBar::changeEvent(event);

	if (event->type() == QEvent::LayoutDirectionChange)
	{
		updateSize();
	}
}

void StatusBarWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOption option;
	option.initFrom(this);

	style()->drawPrimitive(QStyle::PE_PanelStatusBar, &option, &painter, this);
}

void StatusBarWidget::resizeEvent(QResizeEvent *event)
{
	QStatusBar::resizeEvent(event);

	updateSize();
}

void StatusBarWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu(ToolBarWidget::createCustomizationMenu(ToolBarsManager::StatusBar));
	menu->exec(event->globalPos());
	menu->deleteLater();
}

void StatusBarWidget::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Interface_ShowSizeGripOption)
	{
		setSizeGripEnabled(value.toBool());
		updateSize();
	}
}

void StatusBarWidget::toolBarModified(int identifier)
{
	if (identifier == ToolBarsManager::StatusBar)
	{
		setFixedHeight(ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).iconSize);
	}
}

void StatusBarWidget::updateSize()
{
	int offset(0);

	if (isSizeGripEnabled())
	{
		QStyleOption option;
		option.init(this);

		offset = (style()->sizeFromContents(QStyle::CT_SizeGrip, &option, QSize(13, 13), this).expandedTo(QApplication::globalStrut())).height();
	}

	m_toolBar->setFixedSize((width() - offset), ToolBarsManager::getToolBarDefinition(ToolBarsManager::StatusBar).iconSize);
	m_toolBar->move(((layoutDirection() == Qt::RightToLeft) ? offset : 0), 0);
}

}
