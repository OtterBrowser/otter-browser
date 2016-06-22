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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	m_webWidget(NULL),
	m_loadingState(WindowsManager::FinishedLoadingState),
	m_geometryUpdateTimer(0)
{
	WebContentsWidget *contentsWidget(qobject_cast<WebContentsWidget*>(window->getContentsWidget()));

	if (contentsWidget)
	{
		m_webWidget = contentsWidget->getWebWidget();
	}

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

	setAutoFillBackground(true);
	scheduleGeometryUpdate();
	updateLoadingState(window->getLoadingState());
	hide();

	connect(window, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(updateLoadingState(WindowsManager::LoadingState)));
}

void ProgressBarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_geometryUpdateTimer)
	{
		killTimer(m_geometryUpdateTimer);

		m_geometryUpdateTimer = 0;

		QRect geometry(m_webWidget->getProgressBarGeometry());

		if (m_webWidget->getLoadingState() == WindowsManager::OngoingLoadingState)
		{
			if (!isVisible())
			{
				connect(m_webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));
			}

			geometry.translate(0, m_webWidget->pos().y());

			setGeometry(geometry);
			show();
			raise();
		}
		else
		{
			disconnect(m_webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));

			hide();
		}
	}
}

void ProgressBarWidget::updateLoadingState(WindowsManager::LoadingState state)
{
	if (state == m_loadingState || !m_webWidget)
	{
		return;
	}

	if (state != WindowsManager::OngoingLoadingState)
	{
		disconnect(m_webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));

		hide();
	}
	else if (!isVisible())
	{
		scheduleGeometryUpdate();
	}

	m_loadingState = state;
}

void ProgressBarWidget::scheduleGeometryUpdate()
{
	if (m_geometryUpdateTimer == 0 && m_webWidget)
	{
		m_geometryUpdateTimer = startTimer(50);
	}
}

}
