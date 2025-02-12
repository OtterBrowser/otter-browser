/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ProgressToolBarWidget.h"
#include "WebContentsWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/WebWidget.h"
#include "../../../ui/Window.h"

#include <QtWidgets/QHBoxLayout>

namespace Otter
{

ProgressToolBarWidget::ProgressToolBarWidget(Window *window, WebWidget *parent) : QFrame(parent),
	m_webWidget(parent),
	m_window(window),
	m_geometryUpdateTimer(0)
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(new ToolBarWidget(ToolBarsManager::ProgressBar, window, this));

	setFrameStyle(QFrame::StyledPanel);
	setLineWidth(1);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	updateLoadingState(window->getLoadingState());
	hide();

	const MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow)
	{
		connect(mainWindow, &MainWindow::arbitraryActionsStateChanged, this, &ProgressToolBarWidget::handleActionsStateChanged);
	}

	connect(window, &Window::loadingStateChanged, this, &ProgressToolBarWidget::updateLoadingState);
}

void ProgressToolBarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_geometryUpdateTimer)
	{
		killTimer(m_geometryUpdateTimer);

		m_geometryUpdateTimer = 0;

		handleActionsStateChanged({ActionsManager::ShowToolBarAction});
	}
}

void ProgressToolBarWidget::handleActionsStateChanged(const QVector<int> &identifiers)
{
	if (!m_window || !m_webWidget || !identifiers.contains(ActionsManager::ShowToolBarAction))
	{
		return;
	}

	const MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (m_window->getLoadingState() == WebWidget::OngoingLoadingState && mainWindow && mainWindow->getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::ProgressBar}}).isChecked)
	{
		QRect geometry(m_webWidget->getGeometry(true));

		if (!isVisible())
		{
			connect(m_webWidget, &WebWidget::geometryChanged, this, &ProgressToolBarWidget::scheduleGeometryUpdate);
		}

		if (!geometry.isValid())
		{
			geometry = m_webWidget->geometry();
		}

		geometry.setTop(geometry.bottom() - 30);
		geometry.translate(m_webWidget->mapTo(m_window->getContentsWidget(), QPoint(0, 0)));

		setGeometry(geometry);
		show();
		raise();
	}
	else
	{
		disconnect(m_webWidget, &WebWidget::geometryChanged, this, &ProgressToolBarWidget::scheduleGeometryUpdate);

		hide();
	}
}

void ProgressToolBarWidget::updateLoadingState(WebWidget::LoadingState state)
{
	const MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (state == WebWidget::OngoingLoadingState && mainWindow && mainWindow->getActionState(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::ProgressBar}}).isChecked)
	{
		scheduleGeometryUpdate();
	}
	else
	{
		hide();

		if (m_window && m_webWidget)
		{
			disconnect(m_webWidget, &WebWidget::geometryChanged, this, &ProgressToolBarWidget::scheduleGeometryUpdate);
		}
	}
}

void ProgressToolBarWidget::scheduleGeometryUpdate()
{
	if (m_geometryUpdateTimer == 0)
	{
		m_geometryUpdateTimer = startTimer(50);
	}
}

}
