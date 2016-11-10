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

#include "StatusMessageWidget.h"
#include "../../../ui/MainWindow.h"

#include <QtGui/QGuiApplication>
#include <QtWidgets/QStyle>

namespace Otter
{

StatusMessageWidget::StatusMessageWidget(QWidget *parent) : QLabel(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setIndent(style()->pixelMetric(QStyle::PM_ButtonMargin));

	MainWindow *window(MainWindow::findMainWindow(parent));

	if (window)
	{
		connect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setMessage(QString)));
	}
}

void StatusMessageWidget::resizeEvent(QResizeEvent *event)
{
	QLabel::resizeEvent(event);

	setMessage(m_message);
}

void StatusMessageWidget::setMessage(const QString &message)
{
	m_message = message;

	setText(fontMetrics().elidedText(m_message, (QGuiApplication::isLeftToRight() ? Qt::ElideRight : Qt::ElideLeft), width()));
}

}

