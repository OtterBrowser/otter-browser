#include "BookmarksManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
QList<Bookmark> BookmarksManager::m_bookmarks;
QSet<QString> BookmarksManager::m_urls;
int BookmarksManager::m_identifier;

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent)
{
	QTimer::singleShot(250, this, SLOT(load()));
}

void BookmarksManager::load()
{
	m_bookmarks.clear();
	m_identifier = 0;

	QFile file(SettingsManager::getPath() + "/bookmarks.xbel");

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		return;
	}

	QXmlStreamReader reader(file.readAll());

	if (reader.readNextStartElement() && reader.name() == "xbel" && reader.attributes().value("version").toString() == "1.0")
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == "folder" || reader.name() == "bookmark" || reader.name() == "separator")
			{
				m_bookmarks.append(readBookmark(&reader));
			}
			else
			{
				reader.skipCurrentElement();
			}
		}
	}
}

void BookmarksManager::writeBookmark(QXmlStreamWriter *writer, const Bookmark &bookmark)
{
	Q_UNUSED(writer)
	Q_UNUSED(bookmark)

//TODO
}

void BookmarksManager::createInstance(QObject *parent)
{
	m_instance = new BookmarksManager(parent);
}

Bookmark BookmarksManager::readBookmark(QXmlStreamReader *reader) const
{
	Bookmark bookmark;

	if (reader->name() == "folder")
	{
		bookmark.type = FolderBookmark;
		bookmark.identifier = ++m_identifier;

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == "title")
				{
					bookmark.title = reader->readElementText().trimmed();
				}
				else if (reader->name() == "desc")
				{
					bookmark.description = reader->readElementText().trimmed();
				}
				else if (reader->name() == "folder" || reader->name() == "bookmark" || reader->name() == "separator")
				{
					bookmark.children.append(readBookmark(reader));
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == "folder")
			{
				break;
			}
		}
	}
	else if (reader->name() == "bookmark")
	{
		bookmark.type = UrlBookmark;
		bookmark.url = reader->attributes().value("href").toString();

		const QUrl url(bookmark.url);

		if (url.isValid())
		{
			m_urls.insert(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
		}

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == "title")
				{
					bookmark.title = reader->readElementText().trimmed();
				}
				else if (reader->name() == "desc")
				{
					bookmark.description = reader->readElementText().trimmed();
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == "bookmark")
			{
				break;
			}
		}
	}
	else if (reader->name() == "separator")
	{
		bookmark.type = SeparatorBookmark;

		reader->readNext();
	}

	return bookmark;
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
