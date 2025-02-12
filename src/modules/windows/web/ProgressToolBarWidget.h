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

#ifndef OTTER_PROGRESSTOOLBARWIDGET_H
#define OTTER_PROGRESSTOOLBARWIDGET_H

#include "../../../ui/WebWidget.h"

#include <QtWidgets/QFrame>

namespace Otter
{

class WebWidget;
class Window;

class ProgressToolBarWidget final : public QFrame
{
	Q_OBJECT

public:
	explicit ProgressToolBarWidget(Window *window, WebWidget *parent);

public slots:
	void scheduleGeometryUpdate();

protected:
	void timerEvent(QTimerEvent *event) override;

protected slots:
	void handleActionsStateChanged(const QVector<int> &identifiers);
	void updateLoadingState(WebWidget::LoadingState state);

private:
	WebWidget *m_webWidget;
	Window *m_window;
	int m_geometryUpdateTimer;
};

}

#endif
