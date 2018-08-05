/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

void BookmarksManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new BookmarksManager(QCoreApplication::instance());
	}
}

void BookmarksManager::ensureInitialized()
{
	if (!m_instance)
	{
		createInstance();
	}

	if (!m_model)
	{
		m_model = new BookmarksModel(SessionsManager::getWritableDataPath(QLatin1String("bookmarks.xbel")), BookmarksModel::BookmarksMode, m_instance);

		connect(m_model, &BookmarksModel::modelModified, m_instance, &BookmarksManager::scheduleSave);
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
	ensureInitialized();

	if (m_model->hasBookmark(url))
	{
		const QVector<BookmarksModel::Bookmark*> bookmarks(m_model->getBookmarks(url));

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			BookmarksModel::Bookmark *bookmark(bookmarks.at(i));
			bookmark->setData((bookmark->getVisits() + 1), BookmarksModel::VisitsRole);
			bookmark->setData(QDateTime::currentDateTimeUtc(), BookmarksModel::TimeVisitedRole);
		}

		m_instance->scheduleSave();
	}
}

void BookmarksManager::removeBookmark(const QUrl &url)
{
	ensureInitialized();

	const QUrl normalizedUrl(Utils::normalizeUrl(url));

	if (!hasBookmark(normalizedUrl))
	{
		return;
	}

	const QVector<BookmarksModel::Bookmark*> bookmarks(m_model->findUrls(normalizedUrl));

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		bookmarks.at(i)->remove();
	}
}

void BookmarksManager::setLastUsedFolder(BookmarksModel::Bookmark *folder)
{
	m_lastUsedFolder = (folder ? folder->getIdentifier() : 0);
}

BookmarksManager* BookmarksManager::getInstance()
{
	return m_instance;
}

BookmarksModel* BookmarksManager::getModel()
{
	ensureInitialized();

	return m_model;
}

BookmarksModel::Bookmark* BookmarksManager::addBookmark(BookmarksModel::BookmarkType type, const QMap<int, QVariant> &metaData, BookmarksModel::Bookmark *parent, int index)
{
	ensureInitialized();

	return m_model->addBookmark(type, metaData, parent, index);
}

BookmarksModel::Bookmark* BookmarksManager::getBookmark(const QString &text)
{
	ensureInitialized();

	if (text.startsWith(QLatin1Char('#')))
	{
		return m_model->getBookmark(text.mid(1).toULongLong());
	}

	if (text.startsWith(QLatin1String("bookmarks:")))
	{
		return (text.startsWith(QLatin1String("bookmarks:/")) ? m_model->getBookmarkByPath(text.mid(11)) : getBookmark(text.mid(10).toULongLong()));
	}

	return m_model->getBookmarkByKeyword(text);
}

BookmarksModel::Bookmark* BookmarksManager::getBookmark(const QUrl &url)
{
	ensureInitialized();

	const QVector<BookmarksModel::Bookmark*> bookmarks(m_model->getBookmarks(url));

	return (bookmarks.isEmpty() ? nullptr : bookmarks.first());
}

BookmarksModel::Bookmark* BookmarksManager::getBookmark(quint64 identifier)
{
	ensureInitialized();

	if (identifier == 0)
	{
		return m_model->getRootItem();
	}

	return m_model->getBookmark(identifier);
}

BookmarksModel::Bookmark* BookmarksManager::getLastUsedFolder()
{
	ensureInitialized();

	BookmarksModel::Bookmark *folder(m_model->getBookmark(m_lastUsedFolder));

	return ((!folder || folder->getType() != BookmarksModel::FolderBookmark) ? m_model->getRootItem() : folder);
}

QStringList BookmarksManager::getKeywords()
{
	ensureInitialized();

	return m_model->getKeywords();
}

QVector<BookmarksModel::BookmarkMatch> BookmarksManager::findBookmarks(const QString &prefix)
{
	ensureInitialized();

	return m_model->findBookmarks(prefix);
}

bool BookmarksManager::hasBookmark(const QUrl &url)
{
	ensureInitialized();

	return m_model->hasBookmark(url);
}

bool BookmarksManager::hasKeyword(const QString &keyword)
{
	ensureInitialized();

	return m_model->hasKeyword(keyword);
}

}
