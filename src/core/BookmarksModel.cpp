/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksModel.h"
#include "AddonsManager.h"
#include "Console.h"
#include "Utils.h"
#include "WebBackend.h"

#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtWidgets/QMessageBox>

namespace Otter
{

BookmarksItem::BookmarksItem(BookmarkType type) : QStandardItem()
{
	setData(type, BookmarksModel::TypeRole);

	if (type == UrlBookmark || type == SeparatorBookmark)
	{
		setDropEnabled(false);
	}

	if (type == RootBookmark)
	{
		setDragEnabled(false);
	}

	if (type == TrashBookmark)
	{
		setEnabled(false);
		setDragEnabled(false);
	}
}

BookmarksItem::~BookmarksItem()
{
	BookmarksModel *model = getModel();

	if (model)
	{
		model->removeBookmark(this);
	}
}

void BookmarksItem::setData(const QVariant &value, int role)
{
	if (model())
	{
		model()->setData(index(), value, role);
	}
	else
	{
		QStandardItem::setData(value, role);
	}
}

void BookmarksItem::setItemData(const QVariant &value, int role)
{
	QStandardItem::setData(value, role);
}

BookmarksModel* BookmarksItem::getModel() const
{
	return qobject_cast<BookmarksModel*>(model());
}

QStandardItem* BookmarksItem::clone() const
{
	BookmarksItem *item = new BookmarksItem(static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt()));
	item->setData(data(BookmarksModel::UrlRole), BookmarksModel::UrlRole);
	item->setData(data(BookmarksModel::TitleRole), BookmarksModel::TitleRole);
	item->setData(data(BookmarksModel::DescriptionRole), BookmarksModel::DescriptionRole);
	item->setData(data(BookmarksModel::KeywordRole), BookmarksModel::KeywordRole);
	item->setData(data(BookmarksModel::TimeAddedRole), BookmarksModel::TimeAddedRole);
	item->setData(data(BookmarksModel::TimeModifiedRole), BookmarksModel::TimeModifiedRole);
	item->setData(data(BookmarksModel::TimeVisitedRole), BookmarksModel::TimeVisitedRole);
	item->setData(data(BookmarksModel::VisitsRole), BookmarksModel::VisitsRole);

	return item;
}

QVariant BookmarksItem::data(int role) const
{
	if (role == Qt::DecorationRole)
	{
		const BookmarkType type = static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt());

		if (type == RootBookmark || type == FolderBookmark)
		{
			return Utils::getIcon(QLatin1String("inode-directory"));
		}

		if (type == TrashBookmark)
		{
			return Utils::getIcon(QLatin1String("user-trash"));
		}

		if (type == UrlBookmark)
		{
			return AddonsManager::getWebBackend()->getIconForUrl(data(BookmarksModel::UrlRole).toUrl());
		}

		return QVariant();
	}

	if (role == Qt::AccessibleDescriptionRole && static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt()) == SeparatorBookmark)
	{
		return QLatin1String("separator");
	}

	return QStandardItem::data(role);
}

bool BookmarksItem::isInTrash() const
{
	QModelIndex parent = index().parent();

	while (parent.isValid())
	{
		const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt());

		if (type == BookmarksItem::TrashBookmark)
		{
			return true;
		}

		if (type == BookmarksItem::RootBookmark)
		{
			break;
		}

		parent = parent.parent();
	}

	return false;
}

BookmarksModel::BookmarksModel(const QString &path, FormatMode mode, QObject *parent) : QStandardItemModel(parent),
	m_mode(mode)
{
	BookmarksItem *rootItem = new BookmarksItem(BookmarksItem::RootBookmark);
	rootItem->setData(((mode == NotesMode) ? tr("Notes") : tr("Bookmarks")), BookmarksModel::TitleRole);

	BookmarksItem *trashItem = new BookmarksItem(BookmarksItem::TrashBookmark);
	trashItem->setData(tr("Trash"), BookmarksModel::TitleRole);
	trashItem->setEnabled(false);

	appendRow(rootItem);
	appendRow(trashItem);
	setItemPrototype(new BookmarksItem(BookmarksItem::UnknownBookmark));

	QFile file(path);

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		Console::addMessage(tr("Failed to open bookmarks file (%1): %2").arg(path).arg(file.errorString()), OtherMessageCategory, ErrorMessageLevel);

		return;
	}

	QXmlStreamReader reader(file.readAll());

	if (reader.readNextStartElement() && reader.name() == QLatin1String("xbel") && reader.attributes().value(QLatin1String("version")).toString() == QLatin1String("1.0"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("folder") || reader.name() == QLatin1String("bookmark") || reader.name() == QLatin1String("separator"))
			{
				readBookmark(&reader, rootItem);
			}
			else
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError())
			{
				clear();

				QMessageBox::warning(NULL, tr("Error"), tr("Failed to parse bookmarks file. No bookmarks were loaded."), QMessageBox::Close);

				Console::addMessage(tr("Failed to load bookmarks file properly, QXmlStreamReader error code: %1").arg(reader.error()), OtherMessageCategory, ErrorMessageLevel);

				return;
			}
		}
	}
}

void BookmarksModel::trashBookmark(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt());

	if (type != BookmarksItem::RootBookmark && type != BookmarksItem::TrashBookmark)
	{
		if (type == BookmarksItem::SeparatorBookmark || bookmark->isInTrash())
		{
			bookmark->parent()->removeRow(bookmark->row());
		}
		else
		{
			BookmarksItem *trashItem = getTrashItem();

			m_trash[bookmark] = qMakePair(bookmark->parent()->index(), bookmark->row());

			trashItem->appendRow(bookmark->parent()->takeRow(bookmark->row()));
			trashItem->setEnabled(true);
		}
	}
}

void BookmarksModel::restoreBookmark(BookmarksItem *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	BookmarksItem *formerParent = (m_trash.contains(bookmark) ? bookmarkFromIndex(m_trash[bookmark].first) : getRootItem());

	if (!formerParent || static_cast<BookmarksItem::BookmarkType>(formerParent->data(BookmarksModel::TypeRole).toInt()) != BookmarksItem::FolderBookmark)
	{
		formerParent = getRootItem();
	}

	if (m_trash.contains(bookmark))
	{
		formerParent->insertRow(m_trash[bookmark].second, bookmark->parent()->takeRow(bookmark->row()));

		m_trash.remove(bookmark);
	}
	else
	{
		formerParent->appendRow(bookmark->parent()->takeRow(bookmark->row()));
	}

	BookmarksItem *trashItem = getTrashItem();

	trashItem->setEnabled(trashItem->rowCount() > 0);
}

void BookmarksModel::removeBookmark(BookmarksItem *bookmark)
{
	const quint64 identifier = bookmark->data(BookmarksModel::IdentifierRole).toULongLong();

	if (identifier > 0 && m_identifiers.contains(identifier))
	{
		m_identifiers.remove(identifier);
	}

	if (!bookmark->data(BookmarksModel::UrlRole).toString().isEmpty())
	{
		const QUrl url = BookmarksModel::adjustUrl(bookmark->data(BookmarksModel::UrlRole).toUrl());

		if (m_urls.contains(url))
		{
			m_urls[url].removeAll(bookmark);

			if (m_urls[url].isEmpty())
			{
				m_urls.remove(url);
			}
		}
	}

	if (!bookmark->data(BookmarksModel::KeywordRole).toString().isEmpty() && m_keywords.contains(bookmark->data(BookmarksModel::KeywordRole).toString()))
	{
		m_keywords.remove(bookmark->data(BookmarksModel::KeywordRole).toString());
	}
}

void BookmarksModel::readBookmark(QXmlStreamReader *reader, BookmarksItem *parent)
{
	BookmarksItem *bookmark = NULL;

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark = addBookmark(BookmarksItem::FolderBookmark, reader->attributes().value(QLatin1String("id")).toULongLong(), QUrl(), QString(), parent);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate), TimeAddedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate), TimeModifiedRole);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setData(reader->readElementText().trimmed(), TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setData(reader->readElementText().trimmed(), DescriptionRole);
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
											bookmark->setData(reader->readElementText().trimmed(), KeywordRole);
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
		bookmark = addBookmark(BookmarksItem::UrlBookmark,reader->attributes().value(QLatin1String("id")).toULongLong(), reader->attributes().value(QLatin1String("href")).toString(), QString(), parent);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("added")).toString(), Qt::ISODate), TimeAddedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("modified")).toString(), Qt::ISODate), TimeModifiedRole);
		bookmark->setData(QDateTime::fromString(reader->attributes().value(QLatin1String("visited")).toString(), Qt::ISODate), TimeVisitedRole);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setData(reader->readElementText().trimmed(), TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setData(reader->readElementText().trimmed(), DescriptionRole);
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
											bookmark->setData(reader->readElementText().trimmed(), KeywordRole);
										}
										else if (reader->name() == QLatin1String("visits"))
										{
											bookmark->setData(reader->readElementText().toInt(), VisitsRole);
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
		bookmark = addBookmark(BookmarksItem::SeparatorBookmark, 0, QUrl(), QString(), parent);

		reader->readNext();
	}
}

void BookmarksModel::writeBookmark(QXmlStreamWriter *writer, QStandardItem *bookmark) const
{
	if (!bookmark)
	{
		return;
	}

	switch (static_cast<BookmarksItem::BookmarkType>(bookmark->data(TypeRole).toInt()))
	{
		case BookmarksItem::FolderBookmark:
			writer->writeStartElement(QLatin1String("folder"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->data(IdentifierRole).toULongLong()));

			if (bookmark->data(TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->data(TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->data(TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->data(TitleRole).toString());

			if (!bookmark->data(DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->data(DescriptionRole).toString());
			}

			if (m_mode == BookmarksMode && !bookmark->data(KeywordRole).toString().isEmpty())
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), QLatin1String("http://otter-browser.org/otter-xbel-bookmark"));
				writer->writeTextElement(QLatin1String("keyword"), bookmark->data(KeywordRole).toString());
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
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->data(IdentifierRole).toULongLong()));

			if (!bookmark->data(UrlRole).toString().isEmpty())
			{
				writer->writeAttribute(QLatin1String("href"), bookmark->data(UrlRole).toString());
			}

			if (bookmark->data(TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->data(TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->data(TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->data(TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			if (m_mode != NotesMode)
			{
				if (bookmark->data(TimeVisitedRole).toDateTime().isValid())
				{
					writer->writeAttribute(QLatin1String("visited"), bookmark->data(TimeVisitedRole).toDateTime().toString(Qt::ISODate));
				}

				writer->writeTextElement(QLatin1String("title"), bookmark->data(TitleRole).toString());
			}

			if (!bookmark->data(DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->data(DescriptionRole).toString());
			}

			if (m_mode == BookmarksMode && (!bookmark->data(KeywordRole).toString().isEmpty() || bookmark->data(VisitsRole).toInt() > 0))
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), QLatin1String("http://otter-browser.org/otter-xbel-bookmark"));

				if (!bookmark->data(KeywordRole).toString().isEmpty())
				{
					writer->writeTextElement(QLatin1String("keyword"), bookmark->data(KeywordRole).toString());
				}

				if (bookmark->data(VisitsRole).toInt() > 0)
				{
					writer->writeTextElement(QLatin1String("visits"), QString::number(bookmark->data(VisitsRole).toInt()));
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

void BookmarksModel::emptyTrash()
{
	BookmarksItem *trashItem = getTrashItem();

	trashItem->removeRows(0, trashItem->rowCount());
	trashItem->setEnabled(false);

	m_trash.clear();
}

BookmarksItem* BookmarksModel::addBookmark(BookmarksItem::BookmarkType type, quint64 identifier, const QUrl &url, const QString &title, BookmarksItem *parent)
{
	BookmarksItem *bookmark = new BookmarksItem(type);

	if (parent)
	{
		parent->appendRow(bookmark);
	}
	else
	{
		getRootItem()->appendRow(bookmark);
	}

	setData(bookmark->index(), url, BookmarksModel::UrlRole);
	setData(bookmark->index(), title, BookmarksModel::TitleRole);

	if (type != BookmarksItem::SeparatorBookmark && type != BookmarksItem::TrashBookmark && type != BookmarksItem::UnknownBookmark)
	{
		if (identifier == 0 || m_identifiers.contains(identifier))
		{
			identifier = (m_identifiers.isEmpty() ? 1 : (m_identifiers.keys().last() + 1));
		}

		bookmark->setData(identifier, BookmarksModel::IdentifierRole);

		m_identifiers[identifier] = bookmark;
	}

	return bookmark;
}

BookmarksItem* BookmarksModel::bookmarkFromIndex(const QModelIndex &index) const
{
	return dynamic_cast<BookmarksItem*>(itemFromIndex(index));
}

BookmarksItem* BookmarksModel::getBookmark(const QString &keyword) const
{
	if (m_keywords.contains(keyword))
	{
		return m_keywords[keyword];
	}

	return NULL;
}

BookmarksItem* BookmarksModel::getBookmark(quint64 identifier) const
{
	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	return NULL;
}

BookmarksItem* BookmarksModel::getRootItem() const
{
	return dynamic_cast<BookmarksItem*>(item(0, 0));
}

BookmarksItem* BookmarksModel::getTrashItem() const
{
	return dynamic_cast<BookmarksItem*>(item(1, 0));
}

BookmarksItem* BookmarksModel::getItem(const QString &path) const
{
	if (path == QLatin1String("/"))
	{
		return getRootItem();
	}

	QStandardItem *item = getRootItem();
	const QStringList directories = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

	for (int i = 0; i < directories.count(); ++i)
	{
		bool found = false;

		for (int j = 0; j < item->rowCount(); ++j)
		{
			if (item->child(j) && item->child(j)->data(Qt::DisplayRole) == directories.at(i))
			{
				item = item->child(j);

				found = true;

				break;
			}
		}

		if (!found)
		{
			return NULL;
		}
	}

	return dynamic_cast<BookmarksItem*>(item);
}

QMimeData* BookmarksModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QStringList texts;
	QList<QUrl> urls;

	if (indexes.count() == 1)
	{
		mimeData->setProperty("x-item-index", indexes.at(0));
	}

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (indexes.at(i).isValid() && static_cast<BookmarksItem::BookmarkType>(indexes.at(i).data(TypeRole).toInt()) == BookmarksItem::UrlBookmark)
		{
			texts.append(indexes.at(i).data(UrlRole).toString());
			urls.append(indexes.at(i).data(UrlRole).toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	return mimeData;
}

QUrl BookmarksModel::adjustUrl(QUrl url)
{
	url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments | QUrl::StripTrailingSlash);

	if (url.path() == QLatin1String("/"))
	{
		url.setPath(QString());
	}

	return url;
}

QStringList BookmarksModel::mimeTypes() const
{
	return QStringList(QLatin1String("text/uri-list"));
}

QStringList BookmarksModel::getKeywords() const
{
	return m_keywords.keys();
}

QList<BookmarksItem*> BookmarksModel::getBookmarks(const QUrl &url) const
{
	const QUrl adjustedUrl = BookmarksModel::adjustUrl(url);

	if (m_urls.contains(adjustedUrl))
	{
		return m_urls[adjustedUrl];
	}

	return QList<BookmarksItem*>();
}

QList<BookmarksItem*> BookmarksModel::findUrls(const QUrl &url, QStandardItem *branch) const
{
	if (!branch)
	{
		branch = item(0, 0);
	}

	QList<BookmarksItem*> items;

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		BookmarksItem *item = dynamic_cast<BookmarksItem*>(branch->child(i));

		if (item)
		{
			const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(item->data(TypeRole).toInt());

			if (type == BookmarksItem::FolderBookmark)
			{
				items.append(findUrls(url, item));
			}
			else if (type == BookmarksItem::UrlBookmark && item->data(UrlRole).toUrl() == url)
			{
				items.append(item);
			}
		}
	}

	return items;
}

QList<QUrl> BookmarksModel::getUrls() const
{
	return m_urls.keys();
}

bool BookmarksModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt());

	if (type == BookmarksItem::FolderBookmark || type == BookmarksItem::RootBookmark || type == BookmarksItem::TrashBookmark)
	{
		const QModelIndex index = data->property("x-item-index").toModelIndex();

		if (index.isValid())
		{
			QStandardItem *source = itemFromIndex(index);
			QStandardItem *target = itemFromIndex(parent);

			if (source && target)
			{
				if (row < 0)
				{
					target->appendRow(source->parent()->takeRow(source->row()));

					return true;
				}

				int targetRow = row;

				if (source->parent() == target && source->row() < row)
				{
					--targetRow;
				}

				target->insertRow(targetRow, source->parent()->takeRow(source->row()));

				return true;
			}

			return false;
		}

		return QStandardItemModel::dropMimeData(data, action, row, column, parent);
	}

	return false;
}

bool BookmarksModel::save(const QString &path) const
{
	QFile file(path);

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

	QStandardItem *rootItem = item(0, 0);

	for (int i = 0; i < rootItem->rowCount(); ++i)
	{
		writeBookmark(&writer, rootItem->child(i, 0));
	}

	writer.writeEndDocument();

	return true;
}

bool BookmarksModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	BookmarksItem *bookmark = bookmarkFromIndex(index);

	if (!bookmark)
	{
		return QStandardItemModel::setData(index, value, role);
	}

	if (role == BookmarksModel::UrlRole && value.toUrl() != index.data(BookmarksModel::UrlRole).toUrl())
	{
		const QUrl oldUrl = BookmarksModel::adjustUrl(index.data(BookmarksModel::UrlRole).toUrl());
		const QUrl newUrl = BookmarksModel::adjustUrl(value.toUrl());

		if (!oldUrl.isEmpty() && m_urls.contains(oldUrl))
		{
			m_urls[oldUrl].removeAll(bookmark);

			if (m_urls[oldUrl].isEmpty())
			{
				m_urls.remove(oldUrl);
			}
		}

		if (!newUrl.isEmpty())
		{
			if (!m_urls.contains(newUrl))
			{
				m_urls[newUrl] = QList<BookmarksItem*>();
			}

			m_urls[newUrl].append(bookmark);
		}
	}
	else if (role == BookmarksModel::KeywordRole && value.toString() != index.data(BookmarksModel::KeywordRole).toString())
	{
		const QString oldKeyword = index.data(BookmarksModel::KeywordRole).toString();
		const QString newKeyword = value.toString();

		if (!oldKeyword.isEmpty() && m_keywords.contains(oldKeyword))
		{
			m_keywords.remove(oldKeyword);
		}

		if (!newKeyword.isEmpty())
		{
			m_keywords[newKeyword] = bookmark;
		}
	}
	else if (m_mode == NotesMode && role == DescriptionRole)
	{
		const QString title = value.toString().split(QLatin1Char('\n'), QString::SkipEmptyParts).first().left(100);

		setData(index, ((title == value.toString().trimmed()) ? title : title + QStringLiteral("…")), TitleRole);
	}

	bookmark->setItemData(value, role);

	return true;
}

bool BookmarksModel::hasBookmark(const QUrl &url) const
{
	return m_urls.contains(adjustUrl(url));
}

bool BookmarksModel::hasKeyword(const QString &keyword) const
{
	return m_keywords.contains(keyword);
}

bool BookmarksModel::hasUrl(const QUrl &url) const
{
	return m_urls.contains(adjustUrl(url));
}

}
