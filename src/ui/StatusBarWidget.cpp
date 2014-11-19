/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "ActionWidget.h"
#include "toolbars/ZoomWidget.h"
#include "../core/ActionsManager.h"

namespace Otter
{

StatusBarWidget::StatusBarWidget(QWidget *parent) : QStatusBar(parent),
	m_zoomSlider(NULL)
{
}

void StatusBarWidget::setup()
{
	m_zoomSlider = new ZoomWidget(this);

	addPermanentWidget(new ActionWidget(ZoomOutAction, NULL, this));
	addPermanentWidget(m_zoomSlider);
	addPermanentWidget(new ActionWidget(ZoomInAction, NULL, this));

	connect(m_zoomSlider, SIGNAL(requestedZoomChange(int)), this, SIGNAL(requestedZoomChange(int)));
}

void StatusBarWidget::setZoom(int zoom)
{
	m_zoomSlider->setZoom(zoom);
}

void StatusBarWidget::setZoomEnabled(bool enabled)
{
	m_zoomSlider->setEnabled(enabled);
}

}
