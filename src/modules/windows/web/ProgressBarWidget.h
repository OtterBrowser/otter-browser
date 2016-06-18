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

#ifndef OTTER_PROGRESSBARWIDGET_H
#define OTTER_PROGRESSBARWIDGET_H

#include "../../../core/WindowsManager.h"

#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>

namespace Otter
{

class WebWidget;

class ProgressBarWidget : public QFrame
{
	Q_OBJECT

public:
	explicit ProgressBarWidget(WebWidget *webWidget, QWidget *parent = NULL);

public slots:
	void scheduleGeometryUpdate();

protected:
	void timerEvent(QTimerEvent *event);

protected slots:
	void updateLoadingState(WindowsManager::LoadingState state);
	void updateLoadStatus(int elapsedTime, int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);

private:
	WebWidget *m_webWidget;
	QProgressBar *m_progressBar;
	QLabel *m_elementsLabel;
	QLabel *m_totalLabel;
	QLabel *m_speedLabel;
	QLabel *m_elapsedLabel;
	QLabel *m_messageLabel;
	WindowsManager::LoadingState m_loadingState;
	int m_geometryUpdateTimer;
};

}

#endif
