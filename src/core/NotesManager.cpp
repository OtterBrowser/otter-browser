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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "NotesManager.h"
#include "SessionsManager.h"

#include <QtCore/QDateTime>

namespace Otter
{

NotesManager* NotesManager::m_instance(nullptr);
BookmarksModel* NotesManager::m_model(nullptr);

NotesManager::NotesManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void NotesManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new NotesManager(QCoreApplication::instance());
	}
}

void NotesManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (m_model)
		{
			m_model->save(SessionsManager::getWritableDataPath(QLatin1String("notes.xbel")));
		}
	}
}

void NotesManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

NotesManager* NotesManager::getInstance()
{
	return m_instance;
}

BookmarksModel* NotesManager::getModel()
{
	if (!m_model && m_instance)
	{
		m_model = new BookmarksModel(SessionsManager::getWritableDataPath(QLatin1String("notes.xbel")), BookmarksModel::NotesMode, m_instance);

		connect(m_model, &BookmarksModel::modelModified, m_instance, &NotesManager::scheduleSave);
	}

	return m_model;
}

BookmarksModel::Bookmark* NotesManager::addNote(BookmarksModel::BookmarkType type, const QMap<int, QVariant> &metaData, BookmarksModel::Bookmark *parent)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->addBookmark(type, metaData, parent);
}

}
