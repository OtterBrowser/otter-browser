/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent));

	if (mainWindow)
	{
		connect(mainWindow, &MainWindow::statusMessageChanged, this, &StatusMessageWidget::setMessage);
	}
}

void StatusMessageWidget::changeEvent(QEvent *event)
{
	QLabel::changeEvent(event);

	if (event->type() == QEvent::LayoutDirectionChange && !m_message.isEmpty())
	{
		setMessage(m_message);
	}
}

void StatusMessageWidget::resizeEvent(QResizeEvent *event)
{
	QLabel::resizeEvent(event);

	QFont font(parentWidget()->font());

	if (QFontMetrics(font).height() > height())
	{
		int size(height());

		font.setPixelSize(height());

		while (size > 0 && QFontMetrics(font).height() > height())
		{
			font.setPixelSize(--size);
		}
	}

	setFont(font);
	setMessage(m_message);
}

void StatusMessageWidget::setMessage(const QString &message)
{
	m_message = message;

	setText(fontMetrics().elidedText(m_message, (QGuiApplication::isLeftToRight() ? Qt::ElideRight : Qt::ElideLeft), (width() - indent())));
}

}
