/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StartPageWidget.h"
#include "../../../ui/toolbars/SearchWidget.h"

#include <QtQuick/QQuickItem>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QStackedLayout>

namespace Otter
{

StartPageWidget::StartPageWidget(Window *window, QWidget *parent) : QWidget(parent),
	m_quickWidget(new QQuickWidget(this)),
	m_searchWidget(new SearchWidget(window, this))
{
	m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

	QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_quickWidget);

	setLayout(layout);
	updateSize();

	m_searchWidget->show();
	m_searchWidget->raise();
}

void StartPageWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	updateSize();
}

void StartPageWidget::updateSize()
{
	const int searchWidth = qMin(300, (width() - 20));

	m_searchWidget->setFixedWidth(searchWidth);
	m_searchWidget->move(((width() - searchWidth) / 2), 40);

	if (m_quickWidget->rootObject())
	{
		m_quickWidget->rootObject()->setWidth(width());
		m_quickWidget->rootObject()->setHeight(height());
	}
}

}
