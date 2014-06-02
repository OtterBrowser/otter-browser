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

#ifndef OTTER_UTILS_H
#define OTTER_UTILS_H

#include <QtCore/QString>
#include <QtGui/QIcon>

namespace Otter
{

namespace Utils
{

QString elideText(const QString &text, QWidget *widget = NULL, int width = -1);
QString formatConfigurationEntry(const QLatin1String &key, const QString &value, bool quote = false);
QString formatTime(int value);
QString formatUnit(qint64 value, bool isSpeed = false, int precision = 1);
QIcon getIcon(const QLatin1String &name, bool fromTheme = true);

}

}

#endif
