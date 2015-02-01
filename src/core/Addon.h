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

#ifndef OTTER_ADDON_H
#define OTTER_ADDON_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class Addon : public QObject
{
	Q_OBJECT

public:
	explicit Addon(QObject *parent = NULL);

	virtual QString getTitle() const = 0;
	virtual QString getDescription() const = 0;
	virtual QString getVersion() const = 0;
	virtual QUrl getHomePage() const = 0;
	virtual QUrl getUpdateUrl() const = 0;
	virtual QIcon getIcon() const = 0;
};

}

#endif
