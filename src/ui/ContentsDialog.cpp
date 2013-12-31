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

#include "ContentsDialog.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

ContentsDialog::ContentsDialog(QWidget *payload, QWidget *parent) : QWidget(parent),
	m_payload(payload),
	m_titleLabel(new QLabel(payload->windowTitle()))
{
	QFont font = payload->font();
	font.setBold(true);

	QPalette palette = payload->palette();
	palette.setColor(QPalette::Window, palette.color(QPalette::Window).darker(50));

	m_titleLabel->setFont(font);
	m_titleLabel->setPalette(palette);
	m_titleLabel->setAutoFillBackground(true);
	m_titleLabel->setMargin(5);
	m_titleLabel->installEventFilter(this);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_titleLabel);
	layout->addWidget(payload);

	setLayout(layout);

	payload->setWindowFlags(Qt::Widget);
	payload->setFocusPolicy(Qt::StrongFocus);

	adjustSize();
	setAutoFillBackground(true);

	connect(payload, SIGNAL(destroyed()), this, SLOT(deleteLater()));
}

bool ContentsDialog::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_titleLabel)
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			m_offset = mouseEvent->pos();
		}
		else if (event->type() == QEvent::MouseMove)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			move(mapToParent(mouseEvent->pos()) - m_offset);
		}
	}

	return QWidget::eventFilter(object, event);
}

}
