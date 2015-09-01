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

#ifndef OTTER_HANDLERSMANAGER_H
#define OTTER_HANDLERSMANAGER_H

#include <QtCore/QObject>

namespace Otter
{

enum TransferMode
{
	IgnoreTransferMode = 0,
	AskTransferMode,
	OpenTransferMode,
	SaveTransferMode,
	SaveAsTransferMode
};

struct HandlerDefinition
{
	QString openCommand;
	QString downloadsPath;
	TransferMode transferMode;
	bool isExplicit;

	HandlerDefinition() : transferMode(IgnoreTransferMode), isExplicit(true) {}
};

class HandlersManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static HandlersManager* getInstance();
	static HandlerDefinition getHandler(const QString &type);

protected:
	explicit HandlersManager(QObject *parent = NULL);

private:
	static HandlersManager *m_instance;
};

}

#endif
