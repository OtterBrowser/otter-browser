/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ProgressBarWidget.h"
#include "WebContentsWidget.h"
#include "../../../core/Utils.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/WebWidget.h"

#include <QtGui/QPalette>
#include <QtWidgets/QHBoxLayout>

namespace Otter
{

ProgressBarWidget::ProgressBarWidget(Window *window, QWidget *parent) : QFrame(parent),
	m_window(window),
	m_geometryUpdateTimer(0)
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(new ToolBarWidget(ToolBarsManager::ProgressBar, window, this));

	QPalette palette(this->palette());
	palette.setColor(QPalette::Background, palette.color(QPalette::Base));

	setPalette(palette);
	setFrameStyle(QFrame::StyledPanel);
	setLineWidth(1);

	palette.setColor(QPalette::Background, palette.color(QPalette::AlternateBase));

	hide();
	updateLoadingState(window->getLoadingState());
	setAutoFillBackground(true);

	connect(window, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(updateLoadingState(WindowsManager::LoadingState)));
}

void ProgressBarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_geometryUpdateTimer)
	{
		killTimer(m_geometryUpdateTimer);

		m_geometryUpdateTimer = 0;

		if (!m_window)
		{
			return;
		}

		WebContentsWidget *contentsWidget(qobject_cast<WebContentsWidget*>(m_window->getContentsWidget()));

		if (!contentsWidget || !contentsWidget->getWebWidget())
		{
			return;
		}

		WebWidget *webWidget(contentsWidget->getWebWidget());
		QRect geometry(webWidget->getProgressBarGeometry());
		const ToolBarsManager::ToolBarVisibility visibility(ToolBarsManager::getToolBarDefinition(ToolBarsManager::ProgressBar).normalVisibility);

		if (visibility == ToolBarsManager::AlwaysVisibleToolBar || (visibility == ToolBarsManager::AutoVisibilityToolBar && webWidget->getLoadingState() == WindowsManager::OngoingLoadingState))
		{
			if (!isVisible())
			{
				connect(webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));
			}

			geometry.translate(0, webWidget->pos().y());

			setGeometry(geometry);
			show();
			raise();
		}
		else
		{
			disconnect(webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));

			hide();
		}
	}
}

void ProgressBarWidget::updateLoadingState(WindowsManager::LoadingState state)
{
	const ToolBarsManager::ToolBarVisibility visibility(ToolBarsManager::getToolBarDefinition(ToolBarsManager::ProgressBar).normalVisibility);

	if (visibility == ToolBarsManager::AlwaysVisibleToolBar || (visibility == ToolBarsManager::AutoVisibilityToolBar && state == WindowsManager::OngoingLoadingState))
	{
		scheduleGeometryUpdate();
	}
	else
	{
		hide();

		WebContentsWidget *contentsWidget(qobject_cast<WebContentsWidget*>(m_window->getContentsWidget()));

		if (contentsWidget && contentsWidget->getWebWidget())
		{
			disconnect(contentsWidget->getWebWidget(), SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));
		}
	}
}

void ProgressBarWidget::scheduleGeometryUpdate()
{
	if (m_geometryUpdateTimer == 0)
	{
		m_geometryUpdateTimer = startTimer(50);
	}
}

}
