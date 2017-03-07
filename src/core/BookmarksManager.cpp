/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "BookmarksManager.h"
#include "SessionsManager.h"
#include "Utils.h"

#include <QtCore/QDateTime>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance(nullptr);
BookmarksModel* BookmarksManager::m_model(nullptr);
qulonglong BookmarksManager::m_lastUsedFolder(0);

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void BookmarksManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (m_model)
		{
			m_model->save(SessionsManager::getWritableDataPath(QLatin1String("bookmarks.xbel")));
		}
	}
}

void BookmarksManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new BookmarksManager(parent);
	}
}

void BookmarksManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void BookmarksManager::updateVisits(const QUrl &url)
{
	if (!m_model)
	{
		getModel();
	}

	const QUrl adjustedUrl(Utils::normalizeUrl(url));

	if (m_model->hasBookmark(adjustedUrl))
	{
		const QVector<BookmarksItem*> bookmarks(m_model->getBookmarks(adjustedUrl));

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			bookmarks.at(i)->setData((bookmarks.at(i)->data(BookmarksModel::VisitsRole).toInt() + 1), BookmarksModel::VisitsRole);
			bookmarks.at(i)->setData(QDateTime::currentDateTime(), BookmarksModel::TimeVisitedRole);
		}
	}
}

void BookmarksManager::removeBookmark(const QUrl &url)
{
	if (!m_model)
	{
		getModel();
	}

	const QUrl adjustedUrl(Utils::normalizeUrl(url));

	if (!hasBookmark(adjustedUrl))
	{
		return;
	}

	const QVector<BookmarksItem*> items(m_model->findUrls(adjustedUrl));

	for (int i = 0; i < items.count(); ++i)
	{
		items.at(i)->remove();
	}
}

void BookmarksManager::setLastUsedFolder(BookmarksItem *folder)
{
	m_lastUsedFolder = (folder ? folder->data(BookmarksModel::IdentifierRole).toULongLong() : 0);
}

BookmarksManager* BookmarksManager::getInstance()
{
	return m_instance;
}

BookmarksModel* BookmarksManager::getModel()
{
	if (!m_model && m_instance)
	{
		m_model = new BookmarksModel(SessionsManager::getWritableDataPath(QLatin1String("bookmarks.xbel")), BookmarksModel::BookmarksMode, m_instance);

		connect(m_model, SIGNAL(modelModified()), m_instance, SLOT(scheduleSave()));
	}

	return m_model;
}

BookmarksItem* BookmarksManager::addBookmark(BookmarksModel::BookmarkType type, const QUrl &url, const QString &title, BookmarksItem *parent, int index)
{
	if (!m_model)
	{
		getModel();
	}

	BookmarksItem *bookmark(m_model->addBookmark(type, 0, url, title, parent, index));

	if (bookmark)
	{
		bookmark->setData(QDateTime::currentDateTime(), BookmarksModel::TimeAddedRole);
		bookmark->setData(QDateTime::currentDateTime(), BookmarksModel::TimeModifiedRole);
	}

	return bookmark;
}

BookmarksItem* BookmarksManager::getBookmark(const QString &keyword)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->getBookmark(keyword);
}

BookmarksItem* BookmarksManager::getBookmark(quint64 identifier)
{
	if (!m_model)
	{
		getModel();
	}

	if (identifier == 0)
	{
		return m_model->getRootItem();
	}

	return m_model->getBookmark(identifier);
}

BookmarksItem* BookmarksManager::getLastUsedFolder()
{
	BookmarksItem *folder(getModel()->getBookmark(m_lastUsedFolder));

	return ((!folder || static_cast<BookmarksModel::BookmarkType>(folder->data(BookmarksModel::TypeRole).toInt()) != BookmarksModel::FolderBookmark) ? getModel()->getRootItem() : folder);
}

QStringList BookmarksManager::getKeywords()
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->getKeywords();
}

QVector<BookmarksModel::BookmarkMatch> BookmarksManager::findBookmarks(const QString &prefix)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->findBookmarks(prefix);
}

bool BookmarksManager::hasBookmark(const QUrl &url)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->hasBookmark(url);
}

bool BookmarksManager::hasKeyword(const QString &keyword)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->hasKeyword(keyword);
}

}
