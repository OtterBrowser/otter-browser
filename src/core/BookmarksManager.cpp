#include "BookmarksManager.h"
#include "SettingsManager.h"

#include <QtCore/QSet>
#include <QtCore/QUrl>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
QList<Bookmark> BookmarksManager::m_bookmarks;
QSet<QString> BookmarksManager::m_urls;

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent)
{
}

void BookmarksManager::load()
{
//TODO
}

void BookmarksManager::createInstance(QObject *parent)
{
	m_instance = new BookmarksManager(parent);
}

QList<Bookmark> BookmarksManager::getBookmarks()
{
	return m_bookmarks;
}

bool BookmarksManager::hasBookmark(const QString &url)
{
	if (url.isEmpty())
	{
		return false;
	}

	const QUrl bookmark(url);

	if (!bookmark.isValid())
	{
		return false;
	}

	return m_urls.contains(bookmark.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
}

bool BookmarksManager::setBookmarks(const QList<Bookmark> &bookmarks)
{
	m_bookmarks = bookmarks;

//TODO

	return false;
}

}
