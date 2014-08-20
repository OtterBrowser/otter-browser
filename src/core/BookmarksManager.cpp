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

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent),
	 m_saveTimer(0)
{
	QTimer::singleShot(250, this, SLOT(load()));
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

	emit modelModified();
}

void BookmarksManager::load()
{
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
				readBookmark(&reader, m_model->getRootItem());
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

	emit modelModified();
}

void BookmarksManager::readBookmark(QXmlStreamReader *reader, BookmarksItem *parent)
{
	BookmarksItem *bookmark = NULL;

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark = new BookmarksItem(FolderBookmark);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate), BookmarksModel::TimeAddedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate), BookmarksModel::TimeModifiedRole);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setData(reader->readElementText().trimmed(), BookmarksModel::TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setData(reader->readElementText().trimmed(), BookmarksModel::DescriptionRole);
				}
				else if (reader->name() == QLatin1String("folder") || reader->name() == QLatin1String("bookmark") || reader->name() == QLatin1String("separator"))
				{
					readBookmark(reader, bookmark);
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
											bookmark->setData(reader->readElementText().trimmed(), BookmarksModel::KeywordRole);
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
			else if (reader->isEndElement() && reader->name() == QLatin1String("folder"))
			{
				break;
			}
		}
	}
	else if (reader->name() == QLatin1String("bookmark"))
	{
		bookmark = new BookmarksItem(UrlBookmark, reader->attributes().value(QLatin1String("href")).toString());
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate), BookmarksModel::TimeAddedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate), BookmarksModel::TimeModifiedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("visited")).toString(), Qt::ISODate), BookmarksModel::TimeVisitedRole);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setData(reader->readElementText().trimmed(), BookmarksModel::TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setData(reader->readElementText().trimmed(), BookmarksModel::DescriptionRole);
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
											bookmark->setData(reader->readElementText().trimmed(), BookmarksModel::KeywordRole);
										}
										else if (reader->name() == QLatin1String("visits"))
										{
											bookmark->setData(reader->readElementText().toInt(), BookmarksModel::VisitsRole);
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
		bookmark = new BookmarksItem(SeparatorBookmark);

		reader->readNext();
	}

	parent->appendRow(bookmark);
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

BookmarksItem* BookmarksManager::getBookmark(const QString &keyword)
{
	return BookmarksItem::getBookmark(keyword);
}

QStringList BookmarksManager::getKeywords()
{
	return BookmarksItem::getKeywords();
}

QStringList BookmarksManager::getUrls()
{
	return BookmarksItem::getUrls();
}

bool BookmarksManager::hasBookmark(const QString &url)
{
	return BookmarksItem::hasUrl(url);
}

bool BookmarksManager::hasKeyword(const QString &keyword)
{
	return BookmarksItem::hasKeyword(keyword);
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
