/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MacPlatformStyle.h"

#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

namespace Otter
{

MacPlatformStyle::MacPlatformStyle(const QString &name) : Style(name)
{
}

void MacPlatformStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (element == QStyle::PE_IndicatorTabClose)
	{
		painter->save();
		painter->translate(option->rect.topLeft());

		if (option->rect.width() < 12)
		{
			const qreal scale(option->rect.width() / 12.0);

			painter->scale(scale, scale);
		}
		else
		{
			const int offset((option->rect.width() - 12) / 2);

			painter->translate(offset, offset);
		}

		QProxyStyle::drawPrimitive(element, option, painter, widget);

		painter->restore();

		return;
	}

	Style::drawPrimitive(element, option, painter, widget);
}

}
