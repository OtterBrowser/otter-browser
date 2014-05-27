/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "BookmarksManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
QList<BookmarkInformation*> BookmarksManager::m_bookmarks;
QList<BookmarkInformation*> BookmarksManager::m_allBookmarks;
QHash<int, BookmarkInformation*> BookmarksManager::m_pointers;
QSet<QString> BookmarksManager::m_urls;
QHash<QString, BookmarkInformation*> BookmarksManager::m_keywords;
int BookmarksManager::m_identifier;

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent),
	 m_saveTimer(0)
{
	QTimer::singleShot(250, this, SLOT(load()));
}

BookmarksManager::~BookmarksManager()
{
	for (int i = 0; i < m_bookmarks.count(); ++i)
	{
		delete m_bookmarks.at(i);
	}
}

void BookmarksManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		save();
	}
}

void BookmarksManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void BookmarksManager::load()
{
	m_bookmarks.clear();
	m_identifier = 0;

	QFile file(SessionsManager::getProfilePath() + QLatin1String("/bookmarks.xbel"));

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		return;
	}

	QXmlStreamReader reader(file.readAll());

	if (reader.readNextStartElement() && reader.name() == QLatin1String("xbel") && reader.attributes().value(QLatin1String("version")).toString() == QLatin1String("1.0"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("folder") || reader.name() == QLatin1String("bookmark") || reader.name() == QLatin1String("separator"))
			{
				m_bookmarks.append(readBookmark(&reader, 0));
			}
			else
			{
				reader.skipCurrentElement();
			}
		}
	}

	updateIndex();

	emit folderModified(0);
}

void BookmarksManager::writeBookmark(QXmlStreamWriter *writer, BookmarkInformation *bookmark)
{
	switch (bookmark->type)
	{
		case FolderBookmark:
			writer->writeStartElement(QLatin1String("folder"));

			if (bookmark->added.isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->added.toString(Qt::ISODate));
			}

			if (bookmark->modified.isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->modified.toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->title);

			for (int i = 0; i < bookmark->children.count(); ++i)
			{
				writeBookmark(writer, bookmark->children.at(i));
			}

			writer->writeEndElement();

			break;
		case UrlBookmark:
			writer->writeStartElement(QLatin1String("bookmark"));

			if (!bookmark->url.isEmpty())
			{
				writer->writeAttribute(QLatin1String("href"), bookmark->url);
			}

			if (bookmark->added.isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->added.toString(Qt::ISODate));
			}

			if (bookmark->modified.isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->modified.toString(Qt::ISODate));
			}

			if (bookmark->visited.isValid())
			{
				writer->writeAttribute(QLatin1String("visited"), bookmark->visited.toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->title);

			if (!bookmark->description.isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->description);
			}

			if (!bookmark->keyword.isEmpty() || bookmark->visits > 0)
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), QLatin1String("http://otter-browser.org/otter-xbel-bookmark"));

				if (!bookmark->keyword.isEmpty())
				{
					writer->writeTextElement(QLatin1String("keyword"), bookmark->keyword);
				}

				if (bookmark->visits > 0)
				{
					writer->writeTextElement(QLatin1String("visits"), QString().setNum(bookmark->visits));
				}

				writer->writeEndElement();
				writer->writeEndElement();
			}

			writer->writeEndElement();

			break;
		default:
			writer->writeEmptyElement(QLatin1String("separator"));

			break;
	}
}

void BookmarksManager::updateIndex()
{
	updateUrls();
	updateKeywords();
}

void BookmarksManager::updateUrls()
{
	QStringList urls;

	for (int i = 0; i < m_allBookmarks.count(); ++i)
	{
		if (m_allBookmarks.at(i) && m_allBookmarks.at(i)->type == UrlBookmark)
		{
			const QUrl url(m_allBookmarks.at(i)->url);

			if (url.isValid())
			{
				urls.append(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
			}
		}
	}

	m_urls = urls.toSet();
}

void BookmarksManager::updateKeywords()
{
	m_keywords.clear();

	for (int i = 0; i < m_allBookmarks.count(); ++i)
	{
		if (m_allBookmarks.at(i) && !m_allBookmarks.at(i)->keyword.isEmpty())
		{
			m_keywords.insert(m_allBookmarks.at(i)->keyword, m_allBookmarks.at(i));
		}
	}
}

void BookmarksManager::createInstance(QObject *parent)
{
	m_instance = new BookmarksManager(parent);
}

void BookmarksManager::updateVisit(const QUrl &url)
{
	if (hasBookmark(url))
	{
		const QString address = url.toString();

		for (int i = 0; i < m_allBookmarks.count(); ++i)
		{
			if (m_allBookmarks.at(i) && address == m_allBookmarks.at(i)->url)
			{
				m_allBookmarks.at(i)->visited = QDateTime::currentDateTime();

				++m_allBookmarks.at(i)->visits;
			}
		}
	}
}

BookmarksManager* BookmarksManager::getInstance()
{
	return m_instance;
}

QStringList BookmarksManager::getUrls()
{
	return m_urls.toList();
}

QUrl BookmarksManager::getUrlByKeyword(const QString &keyword)
{
	return m_keywords.contains(keyword)? QUrl(m_keywords.value(keyword)->url): QUrl();
}

BookmarkInformation *BookmarksManager::readBookmark(QXmlStreamReader *reader, int parent)
{
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->parent = parent;

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark->type = FolderBookmark;
		bookmark->identifier = ++m_identifier;
		bookmark->added = QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate);
		bookmark->modified = QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->title = reader->readElementText().trimmed();
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->description = reader->readElementText().trimmed();
				}
				else if (reader->name() == QLatin1String("folder") || reader->name() == QLatin1String("bookmark") || reader->name() == QLatin1String("separator"))
				{
					bookmark->children.append(readBookmark(reader, bookmark->identifier));
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == QLatin1String("folder"))
			{
				break;
			}
		}

		m_pointers[bookmark->identifier] = bookmark;
	}
	else if (reader->name() == QLatin1String("bookmark"))
	{
		bookmark->type = UrlBookmark;
		bookmark->url = reader->attributes().value(QLatin1String("href")).toString();
		bookmark->added = QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate);
		bookmark->modified = QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate);
		bookmark->visited = QDateTime::fromString(reader->attributes().value(QLatin1String("visited")).toString(), Qt::ISODate);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->title = reader->readElementText().trimmed();
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->description = reader->readElementText().trimmed();
				}
				else if (reader->name() == QLatin1String("info"))
				{
					while (reader->readNext())
					{
						if (reader->isStartElement())
						{
							if (reader->name() == QLatin1String("metadata") && reader->attributes().value(QLatin1String("owner")).toString().startsWith("http://otter-browser.org/"))
							{
								while (reader->readNext())
								{
									if (reader->isStartElement())
									{
										if (reader->name() == QLatin1String("keyword"))
										{
											bookmark->keyword = reader->readElementText().trimmed();
										}
										else if (reader->name() == QLatin1String("visits"))
										{
											bookmark->visits = reader->readElementText().toInt();
										}
										else
										{
											reader->skipCurrentElement();
										}
									}
									else if (reader->isEndElement() && reader->name() == QLatin1String("metadata"))
									{
										break;
									}
								}
							}
							else
							{
								reader->skipCurrentElement();
							}
						}
						else if (reader->isEndElement() && reader->name() == QLatin1String("info"))
						{
							break;
						}
					}
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == QLatin1String("bookmark"))
			{
				break;
			}
		}
	}
	else if (reader->name() == QLatin1String("separator"))
	{
		bookmark->type = SeparatorBookmark;

		reader->readNext();
	}

	m_allBookmarks.append(bookmark);

	return bookmark;
}

QList<BookmarkInformation*> BookmarksManager::getBookmarks()
{
	return m_bookmarks;
}

QList<BookmarkInformation*> BookmarksManager::getFolder(int folder)
{
	if (folder == 0)
	{
		return m_bookmarks;
	}

	if (m_pointers.contains(folder))
	{
		return m_pointers[folder]->children;
	}

	return QList<BookmarkInformation*>();
}

bool BookmarksManager::addBookmark(BookmarkInformation *bookmark, int folder, int index)
{
	if (!bookmark || (folder != 0 && !m_pointers.contains(folder)))
	{
		return false;
	}

	const QUrl url(bookmark->url);

	if (url.isValid())
	{
		m_urls.insert(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
	}

	if (!bookmark->keyword.isEmpty())
	{
		m_keywords.insert(bookmark->keyword, bookmark);
	}

	if (bookmark->type == FolderBookmark)
	{
		bookmark->identifier = ++m_identifier;

		m_pointers[bookmark->identifier] = bookmark;
	}

	bookmark->parent = folder;
	bookmark->added = QDateTime::currentDateTime();
	bookmark->modified = bookmark->added;

	if (folder == 0)
	{
		m_bookmarks.insert(((index < 0) ? m_bookmarks.count() : index), bookmark);
	}
	else
	{
		m_pointers[folder]->children.insert(((index < 0) ? m_pointers[folder]->children.count() : index), bookmark);
	}

	m_allBookmarks.append(bookmark);

	emit m_instance->folderModified(folder);

	return save();
}

bool BookmarksManager::updateBookmark(BookmarkInformation *bookmark)
{
	if (bookmark && m_allBookmarks.contains(bookmark))
	{
		bookmark->modified = QDateTime::currentDateTime();

		updateIndex();

		emit m_instance->folderModified(bookmark->parent);

		return save();
	}

	return false;
}

bool BookmarksManager::deleteBookmark(BookmarkInformation *bookmark, bool notify)
{
	if (!bookmark || !m_allBookmarks.contains(bookmark))
	{
		return false;
	}

	const int folder = bookmark->parent;

	for (int i = 0; i < bookmark->children.count(); ++i)
	{
		deleteBookmark(bookmark->children.at(i), false);
	}

	m_bookmarks.removeAll(bookmark);
	m_allBookmarks.removeAll(bookmark);

	if (folder > 0 && m_pointers.contains(folder))
	{
		m_pointers[folder]->children.removeAll(bookmark);
	}

	if (bookmark->type == FolderBookmark)
	{
		m_pointers.remove(bookmark->identifier);
	}

	delete bookmark;

	if (notify)
	{
		m_instance->scheduleSave();

		updateIndex();

		emit m_instance->folderModified(folder);
	}

	return true;
}

bool BookmarksManager::deleteBookmark(const QUrl &url)
{
	if (!hasBookmark(url))
	{
		return false;
	}

	const QString bookmarkUrl = url.toString(QUrl::RemovePassword);

	for (int i = 0; i < m_allBookmarks.count(); ++i)
	{
		if (m_allBookmarks.at(i) && m_allBookmarks.at(i)->type == UrlBookmark && m_allBookmarks.at(i)->url == bookmarkUrl)
		{
			deleteBookmark(m_allBookmarks.at(i));
		}
	}

	return true;
}

bool BookmarksManager::hasBookmark(const QString &url)
{
	if (url.isEmpty())
	{
		return false;
	}

	return hasBookmark(QUrl(url));
}

bool BookmarksManager::hasBookmark(const QUrl &url)
{
	if (!url.isValid())
	{
		return false;
	}

	return m_urls.contains(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
}

bool BookmarksManager::save(const QString &path)
{
	QFile file(path.isEmpty() ? SessionsManager::getProfilePath() + QLatin1String("/bookmarks.xbel") : path);

	if (!file.open(QFile::WriteOnly))
	{
		return false;
	}

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.setAutoFormattingIndent(-1);
	writer.writeStartDocument();
	writer.writeDTD(QLatin1String("<!DOCTYPE xbel>"));
	writer.writeStartElement(QLatin1String("xbel"));
	writer.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));

	for (int i = 0; i < m_bookmarks.count(); ++i)
	{
		writeBookmark(&writer, m_bookmarks.at(i));
	}

	writer.writeEndDocument();

	return true;
}

}
