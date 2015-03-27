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

#ifndef OTTER_NOTESMANAGER_H
#define OTTER_NOTESMANAGER_H

#include "BookmarksModel.h"

#include <QtCore/QObject>

namespace Otter
{

class NotesManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static NotesManager* getInstance();
	static BookmarksModel* getModel();

protected:
	explicit NotesManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);

protected slots:
	void scheduleSave();

private:
	int m_saveTimer;

	static NotesManager *m_instance;
	static BookmarksModel *m_model;

signals:
	void modelModified();
};

}

#endif
