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

#include "Style.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleFactory>

namespace Otter
{

Style::Style(const QString &name) : QProxyStyle(name.isEmpty() ? nullptr : QStyleFactory::create(name))
{
}

void Style::drawDropZone(const QLine &line, QPainter *painter)
{
	painter->save();
	painter->setPen(QPen(QGuiApplication::palette().text(), 3, Qt::DotLine));
	painter->drawLine(line);
	painter->setPen(QPen(QGuiApplication::palette().text(), 9, Qt::SolidLine, Qt::RoundCap));
	painter->drawPoint(line.p1());
	painter->drawPoint(line.p2());
	painter->restore();
}

}
