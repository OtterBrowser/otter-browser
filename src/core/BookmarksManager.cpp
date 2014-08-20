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
#include "BookmarksModel.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
BookmarksModel* BookmarksManager::m_model = NULL;
QList<BookmarkInformation*> BookmarksManager::m_bookmarks;
QList<BookmarkInformation*> BookmarksManager::m_allBookmarks;
QHash<int, BookmarkInformation*> BookmarksManager::m_pointers;
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

void BookmarksManager::createInstance(QObject *parent)
{
	m_instance = new BookmarksManager(parent);
	m_model = new BookmarksModel(m_instance);
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
				m_bookmarks.append(readBookmark(&reader, m_model->getRootItem(), 0));
			}
			else
			{
				reader.skipCurrentElement();
			}
		}
	}

	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), m_instance, SLOT(scheduleSave()));
	connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));
	connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));
	connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), m_instance, SLOT(scheduleSave()));

	emit folderModified(0);
}

void BookmarksManager::writeBookmark(QXmlStreamWriter *writer, QStandardItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	switch (static_cast<BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()))
	{
		case FolderBookmark:
			writer->writeStartElement(QLatin1String("folder"));

			if (bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(BookmarksModel::TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->data(BookmarksModel::TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->data(BookmarksModel::TitleRole).toString());

			if (!bookmark->data(BookmarksModel::DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->data(BookmarksModel::DescriptionRole).toString());
			}

			if (!bookmark->data(BookmarksModel::KeywordRole).toString().isEmpty())
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), QLatin1String("http://otter-browser.org/otter-xbel-bookmark"));
				writer->writeTextElement(QLatin1String("keyword"), bookmark->data(BookmarksModel::KeywordRole).toString());
				writer->writeEndElement();
				writer->writeEndElement();
			}

			for (int i = 0; i < bookmark->rowCount(); ++i)
			{
				writeBookmark(writer, bookmark->child(i, 0));
			}

			writer->writeEndElement();

			break;
		case UrlBookmark:
			writer->writeStartElement(QLatin1String("bookmark"));

			if (!bookmark->data(BookmarksModel::UrlRole).toString().isEmpty())
			{
				writer->writeAttribute(QLatin1String("href"), bookmark->data(BookmarksModel::UrlRole).toString());
			}

			if (bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(BookmarksModel::TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->data(BookmarksModel::TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(BookmarksModel::TimeVisitedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("visited"), bookmark->data(BookmarksModel::TimeVisitedRole).toDateTime().toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->data(BookmarksModel::TitleRole).toString());

			if (!bookmark->data(BookmarksModel::DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->data(BookmarksModel::DescriptionRole).toString());
			}

			if (!bookmark->data(BookmarksModel::KeywordRole).toString().isEmpty() || bookmark->data(BookmarksModel::VisitsRole).toInt() > 0)
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), QLatin1String("http://otter-browser.org/otter-xbel-bookmark"));

				if (!bookmark->data(BookmarksModel::KeywordRole).toString().isEmpty())
				{
					writer->writeTextElement(QLatin1String("keyword"), bookmark->data(BookmarksModel::KeywordRole).toString());
				}

				if (bookmark->data(BookmarksModel::VisitsRole).toInt() > 0)
				{
					writer->writeTextElement(QLatin1String("visits"), QString::number(bookmark->data(BookmarksModel::VisitsRole).toInt()));
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

void BookmarksManager::updateVisits(const QString &url)
{
	if (BookmarksItem::hasBookmark(url))
	{
		const QList<BookmarksItem*> bookmarks = BookmarksItem::getBookmarks(url);

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			bookmarks.at(i)->setData(bookmarks.at(i)->data(BookmarksModel::VisitsRole).toInt(), BookmarksModel::VisitsRole);
			bookmarks.at(i)->setData(QDateTime::currentDateTime(), BookmarksModel::TimeVisitedRole);
		}
	}
}

void BookmarksManager::addBookmark(BookmarkInformation *bookmark, int folder, int index)
{
	if (!bookmark || (folder != 0 && !m_pointers.contains(folder)))
	{
		return;
	}

	if (bookmark->type == FolderBookmark)
	{
		bookmark->identifier = ++m_identifier;

		m_pointers[bookmark->identifier] = bookmark;
	}

	bookmark->parent = folder;

	if (!bookmark->added.isValid())
	{
		bookmark->added = QDateTime::currentDateTime();
	}

	if (!bookmark->modified.isValid())
	{
		bookmark->modified = bookmark->added;
	}

	if (folder == 0)
	{
		m_bookmarks.insert(((index < 0) ? m_bookmarks.count() : index), bookmark);
	}
	else
	{
		m_pointers[folder]->children.insert(((index < 0) ? m_pointers[folder]->children.count() : index), bookmark);
	}

	m_allBookmarks.append(bookmark);

	BookmarksItem *item = new BookmarksItem(bookmark->type, bookmark->url, bookmark->title);
	item->setData(bookmark->description, BookmarksModel::DescriptionRole);
	item->setData(bookmark->keyword, BookmarksModel::KeywordRole);
	item->setData(bookmark->added, BookmarksModel::TimeAddedRole);
	item->setData(bookmark->modified, BookmarksModel::TimeModifiedRole);
	item->setData(bookmark->visited, BookmarksModel::TimeVisitedRole);
	item->setData(bookmark->visits, BookmarksModel::VisitsRole);

	if (folder == 0)
	{
		m_model->invisibleRootItem()->appendRow(item);
	}
	else if (m_pointers[folder]->item)
	{
		m_pointers[folder]->item->appendRow(item);
	}

	bookmark->item = item;

	emit m_instance->folderModified(folder);

	m_instance->scheduleSave();
}

void BookmarksManager::deleteBookmark(const QString &url)
{
	if (!hasBookmark(url))
	{
		return;
	}

	const QList<QStandardItem*> items = m_model->findUrls(url);

	for (int i = 0; i < items.count(); ++i)
	{
		items.at(i)->parent()->removeRow(items.at(i)->row());
	}
}

BookmarksManager* BookmarksManager::getInstance()
{
	return m_instance;
}

BookmarksModel* BookmarksManager::getModel()
{
	return m_model;
}

BookmarkInformation* BookmarksManager::getBookmark(const int identifier)
{
	return (m_pointers.contains(identifier) ? m_pointers[identifier] : NULL);
}

BookmarksItem* BookmarksManager::getBookmark(const QString &keyword)
{
	return BookmarksItem::getBookmark(keyword);
}

BookmarkInformation* BookmarksManager::readBookmark(QXmlStreamReader *reader, BookmarksItem *parent, int parentIdentifier)
{
	BookmarksItem *item = NULL;
	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->parent = parentIdentifier;

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark->type = FolderBookmark;
		bookmark->identifier = ++m_identifier;
		bookmark->added = QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate);
		bookmark->modified = QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate);

		item = new BookmarksItem(FolderBookmark);

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
					bookmark->children.append(readBookmark(reader, item, bookmark->identifier));
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
		item = new BookmarksItem(UrlBookmark);

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
		item = new BookmarksItem(SeparatorBookmark);

		bookmark->type = SeparatorBookmark;

		reader->readNext();
	}

	m_allBookmarks.append(bookmark);

	bookmark->item = item;

	parent->appendRow(item);

	item->setData(bookmark->title, BookmarksModel::TitleRole);
	item->setData(bookmark->description, BookmarksModel::DescriptionRole);
	item->setData(bookmark->url, BookmarksModel::UrlRole);
	item->setData(bookmark->keyword, BookmarksModel::KeywordRole);
	item->setData(bookmark->added, BookmarksModel::TimeAddedRole);
	item->setData(bookmark->modified, BookmarksModel::TimeModifiedRole);
	item->setData(bookmark->visited, BookmarksModel::TimeVisitedRole);
	item->setData(bookmark->visits, BookmarksModel::VisitsRole);

	return bookmark;
}

QStringList BookmarksManager::getKeywords()
{
	return BookmarksItem::getKeywords();
}

QStringList BookmarksManager::getUrls()
{
	return BookmarksItem::getUrls();
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

bool BookmarksManager::hasBookmark(const QString &url)
{
	return BookmarksItem::hasUrl(url);
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

	for (int i = 0; i < m_model->getRootItem()->rowCount(); ++i)
	{
		writeBookmark(&writer, m_model->getRootItem()->child(i, 0));
	}

	writer.writeEndDocument();

	return true;
}

}
