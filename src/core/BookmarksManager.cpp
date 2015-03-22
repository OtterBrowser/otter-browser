/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Console.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtWidgets/QMessageBox>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
BookmarksModel* BookmarksManager::m_model = NULL;

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

		save();
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

	emit modelModified();
}

void BookmarksManager::readBookmark(QXmlStreamReader *reader, BookmarksItem *parent)
{
	BookmarksItem *bookmark = NULL;

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark = new BookmarksItem(BookmarksItem::FolderBookmark, reader->attributes().value(QLatin1String("id")).toULongLong());
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
			else if (reader->hasError())
			{
				return;
			}
		}
	}
	else if (reader->name() == QLatin1String("bookmark"))
	{
		bookmark = new BookmarksItem(BookmarksItem::UrlBookmark,reader->attributes().value(QLatin1String("id")).toULongLong(), reader->attributes().value(QLatin1String("href")).toString());
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
			else if (reader->hasError())
			{
				return;
			}
		}
	}
	else if (reader->name() == QLatin1String("separator"))
	{
		bookmark = new BookmarksItem(BookmarksItem::SeparatorBookmark);

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

	switch (static_cast<BookmarksItem::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()))
	{
		case BookmarksItem::FolderBookmark:
			writer->writeStartElement(QLatin1String("folder"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->data(BookmarksModel::IdentifierRole).toULongLong()));

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
		case BookmarksItem::UrlBookmark:
			writer->writeStartElement(QLatin1String("bookmark"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->data(BookmarksModel::IdentifierRole).toULongLong()));

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

void BookmarksManager::updateVisits(const QUrl &url)
{
	const QUrl adjustedUrl = BookmarksItem::adjustUrl(url);

	if (BookmarksItem::hasBookmark(adjustedUrl))
	{
		const QList<BookmarksItem*> bookmarks = BookmarksItem::getBookmarks(adjustedUrl);

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			bookmarks.at(i)->setData((bookmarks.at(i)->data(BookmarksModel::VisitsRole).toInt() + 1), BookmarksModel::VisitsRole);
			bookmarks.at(i)->setData(QDateTime::currentDateTime(), BookmarksModel::TimeVisitedRole);
		}
	}
}

void BookmarksManager::deleteBookmark(const QUrl &url)
{
	const QUrl adjustedUrl = BookmarksItem::adjustUrl(url);

	if (!hasBookmark(adjustedUrl))
	{
		return;
	}

	const QList<QStandardItem*> items = m_model->findUrls(adjustedUrl);

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
	if (!m_model && m_instance)
	{
		QFile file(SessionsManager::getProfilePath() + QLatin1String("/bookmarks.xbel"));

		m_model = new BookmarksModel(m_instance);

		if (!file.exists())
		{
			connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));

			return m_model;
		}

		if (!file.open(QFile::ReadOnly | QFile::Text))
		{
			Console::addMessage(tr("Failed to open bookmarks file: %0").arg(file.errorString()), OtherMessageCategory, ErrorMessageLevel);

			return m_model;
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

				if (reader.hasError())
				{
					m_model->clear();

					QMessageBox::warning(NULL, tr("Error"), tr("Failed to parse bookmarks file. No bookmarks were loaded."), QMessageBox::Close);
					Console::addMessage(tr("Failed to load bookmarks file properly, QXmlStreamReader error code: %1").arg(reader.error()), OtherMessageCategory, ErrorMessageLevel);

					return m_model;
				}
			}
		}

		connect(m_model, SIGNAL(itemChanged(QStandardItem*)), m_instance, SLOT(scheduleSave()));
		connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));
		connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));
		connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), m_instance, SLOT(scheduleSave()));

		emit m_instance->modelModified();
	}

	return m_model;
}

BookmarksItem* BookmarksManager::getBookmark(const QString &keyword)
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::getBookmark(keyword);
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

	return BookmarksItem::getBookmark(identifier);
}

QStringList BookmarksManager::getKeywords()
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::getKeywords();
}

QList<QUrl> BookmarksManager::getUrls()
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::getUrls();
}

bool BookmarksManager::hasBookmark(const QUrl &url)
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::hasUrl(url);
}

bool BookmarksManager::hasKeyword(const QString &keyword)
{
	if (!m_model)
	{
		getModel();
	}

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
