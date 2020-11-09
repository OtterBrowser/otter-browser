/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "DateTimeTextLabelWidget.h"
#include "../core/Utils.h"

#include <QtCore/QEvent>

namespace Otter
{

DateTimeTextLabelWidget::DateTimeTextLabelWidget(QWidget *parent) : TextLabelWidget(parent)
{
}

void DateTimeTextLabelWidget::changeEvent(QEvent *event)
{
	TextLabelWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		setDateTime(m_dateTime);
	}
}

void DateTimeTextLabelWidget::clear()
{
	m_dateTime = {};

	TextLabelWidget::clear();
}

void DateTimeTextLabelWidget::setDateTime(const QDateTime &dateTime)
{
	m_dateTime = dateTime;

	setText(dateTime.isValid() ? Utils::formatDateTime(dateTime) : tr("Unknown"));
}

}
