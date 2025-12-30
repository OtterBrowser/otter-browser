/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "BookmarksModel.h"
#include "Console.h"
#include "FeedsManager.h"
#include "HistoryManager.h"
#include "SessionsManager.h"
#include "ThemesManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtCore/QSaveFile>
#include <QtCore/QTimeZone>
#include <QtWidgets/QMessageBox>

namespace Otter
{

BookmarksModel::Bookmark::Bookmark() = default;

void BookmarksModel::Bookmark::remove()
{
	BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

	if (model)
	{
		model->removeBookmark(this);
	}
	else
	{
		delete this;
	}
}

void BookmarksModel::Bookmark::setData(const QVariant &value, int role)
{
	QStandardItemModel *model(this->model());

	if (model && qobject_cast<BookmarksModel*>(model))
	{
		model->setData(index(), value, role);
	}
	else
	{
		QStandardItem::setData(value, role);
	}
}

void BookmarksModel::Bookmark::setItemData(const QVariant &value, int role)
{
	QStandardItem::setData(value, role);
}

QStandardItem* BookmarksModel::Bookmark::clone() const
{
	Bookmark *bookmark(new Bookmark());

	for (int i = TitleRole; i < UserRole; ++i)
	{
		if (i != IdentifierRole && i != IsTrashedRole)
		{
			bookmark->setData(getRawData(i), i);
		}
	}

	return bookmark;
}

BookmarksModel::Bookmark* BookmarksModel::Bookmark::getParent() const
{
	BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

	if (model)
	{
		return model->getBookmark(index().parent());
	}

	return nullptr;
}

BookmarksModel::Bookmark* BookmarksModel::Bookmark::getChild(int index) const
{
	BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

	if (model)
	{
		return model->getBookmark(model->index(index, 0, this->index()));
	}

	return nullptr;
}

QString BookmarksModel::Bookmark::getTitle() const
{
	return data(TitleRole).toString();
}

QString BookmarksModel::Bookmark::getDescription() const
{
	return QStandardItem::data(DescriptionRole).toString();
}

QString BookmarksModel::Bookmark::getKeyword() const
{
	return QStandardItem::data(KeywordRole).toString();
}

QUrl BookmarksModel::Bookmark::getUrl() const
{
	return QStandardItem::data(UrlRole).toUrl();
}

QDateTime BookmarksModel::Bookmark::getTimeAdded() const
{
	return QStandardItem::data(TimeAddedRole).toDateTime();
}

QDateTime BookmarksModel::Bookmark::getTimeModified() const
{
	return QStandardItem::data(TimeModifiedRole).toDateTime();
}

QDateTime BookmarksModel::Bookmark::getTimeVisited() const
{
	return QStandardItem::data(TimeVisitedRole).toDateTime();
}

QIcon BookmarksModel::Bookmark::getIcon() const
{
	return data(Qt::DecorationRole).value<QIcon>();
}

QVariant BookmarksModel::Bookmark::data(int role) const
{
	if (role == Qt::DisplayRole)
	{
		const BookmarkType type(getType());

		switch (type)
		{
			case RootBookmark:
				{
					const BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

					if (model && model->getFormatMode() == NotesMode)
					{
						return QCoreApplication::translate("Otter::BookmarksModel", "Notes");
					}
				}

				return QCoreApplication::translate("Otter::BookmarksModel", "Bookmarks");
			case TrashBookmark:
				return QCoreApplication::translate("Otter::BookmarksModel", "Trash");
			case FeedBookmark:
			case FolderBookmark:
			case UrlBookmark:
				if (QStandardItem::data(role).isNull())
				{
					if (type == UrlBookmark)
					{
						const BookmarksModel *model(qobject_cast<BookmarksModel*>(this->model()));

						if (model && model->getFormatMode() == NotesMode)
						{
							const QString text(data(DescriptionRole).toString());
							const int newLinePosition(text.indexOf(QLatin1Char('\n')));

							if (newLinePosition > 0 && newLinePosition < (text.length() - 1))
							{
								return text.left(newLinePosition) + QStringLiteral("…");
							}

							return text;
						}
					}

					return QCoreApplication::translate("Otter::BookmarksModel", "(Untitled)");
				}

				break;
			default:
				break;
		}

		return QStandardItem::data(Qt::DisplayRole);
	}

	if (role == Qt::DecorationRole)
	{
		switch (getType())
		{
			case RootBookmark:
			case FolderBookmark:
				return ThemesManager::createIcon(QLatin1String("inode-directory"));
			case FeedBookmark:
				return ThemesManager::createIcon(QLatin1String("application-rss+xml"));
			case TrashBookmark:
				return ThemesManager::createIcon(QLatin1String("user-trash"));
			case UrlBookmark:
				return HistoryManager::getIcon(data(UrlRole).toUrl());
			default:
				break;
		}

		return {};
	}

	if (role == Qt::AccessibleDescriptionRole && getType() == SeparatorBookmark)
	{
		return QLatin1String("separator");
	}

	if (role == IsTrashedRole)
	{
		QModelIndex parent(index().parent());

		while (parent.isValid())
		{
			const BookmarkType type(static_cast<BookmarkType>(parent.data(TypeRole).toInt()));

			if (type == RootBookmark)
			{
				return false;
			}

			if (type == TrashBookmark)
			{
				return true;
			}

			parent = parent.parent();
		}

		return false;
	}

	return QStandardItem::data(role);
}

QVariant BookmarksModel::Bookmark::getRawData(int role) const
{
	return QStandardItem::data(role);
}

QVector<QUrl> BookmarksModel::Bookmark::getUrls() const
{
	QVector<QUrl> urls;

	if (getType() == UrlBookmark)
	{
		urls.append(data(UrlRole).toUrl());
	}

	for (int i = 0; i < rowCount(); ++i)
	{
		const Bookmark *bookmark(getChild(i));

		if (!bookmark)
		{
			continue;
		}

		switch (bookmark->getType())
		{
			case FolderBookmark:
				urls.append(bookmark->getUrls());

				break;
			case UrlBookmark:
				urls.append(bookmark->getUrl());

				break;
			default:
				break;
		}
	}

	return urls;
}

quint64 BookmarksModel::Bookmark::getIdentifier() const
{
	return QStandardItem::data(IdentifierRole).toULongLong();
}

BookmarksModel::BookmarkType BookmarksModel::Bookmark::getType() const
{
	return static_cast<BookmarkType>(QStandardItem::data(TypeRole).toInt());
}

int BookmarksModel::Bookmark::getVisits() const
{
	return QStandardItem::data(VisitsRole).toInt();
}

bool BookmarksModel::Bookmark::hasChildren() const
{
	return (rowCount() > 0);
}

bool BookmarksModel::Bookmark::isFolder(BookmarksModel::BookmarkType type)
{
	switch (type)
	{
		case RootBookmark:
		case TrashBookmark:
		case FolderBookmark:
			return true;
		default:
			return false;
	}

	return false;
}

bool BookmarksModel::Bookmark::isFolder() const
{
	return isFolder(getType());
}

bool BookmarksModel::Bookmark::isAncestorOf(Bookmark *child) const
{
	if (child == nullptr || child == this)
	{
		return false;
	}

	QStandardItem *parent(child->parent());

	while (parent)
	{
		if (parent == this)
		{
			return true;
		}

		parent = parent->parent();
	}

	return false;
}

bool BookmarksModel::Bookmark::operator<(const QStandardItem &other) const
{
	const BookmarkType type(getType());

	if (type == RootBookmark || type == TrashBookmark)
	{
		return false;
	}

	return QStandardItem::operator<(other);
}

BookmarksModel::BookmarksModel(const QString &path, FormatMode mode, QObject *parent) : QStandardItemModel(parent),
	m_rootItem(new Bookmark()),
	m_trashItem(new Bookmark()),
	m_importTargetItem(nullptr),
	m_mode(mode)
{
	m_rootItem->setData(RootBookmark, TypeRole);
	m_rootItem->setDragEnabled(false);
	m_trashItem->setData(TrashBookmark, TypeRole);
	m_trashItem->setDragEnabled(false);
	m_trashItem->setEnabled(false);

	appendRow(m_rootItem);
	appendRow(m_trashItem);
	setItemPrototype(new Bookmark());

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);
	const bool isNotes(m_mode == NotesMode);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage((isNotes ? tr("Failed to open notes file: %1") : tr("Failed to open bookmarks file: %1")).arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

		return;
	}

	QXmlStreamReader reader(&file);

	if (reader.readNextStartElement() && reader.name() == QLatin1String("xbel") && reader.attributes().value(QLatin1String("version")).toString() == QLatin1String("1.0"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("folder") || reader.name() == QLatin1String("bookmark") || reader.name() == QLatin1String("separator"))
			{
				readBookmark(&reader, m_rootItem);
			}
			else
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError())
			{
				m_rootItem->removeRows(0, m_rootItem->rowCount());

				Console::addMessage((isNotes ? tr("Failed to load notes file: %1") : tr("Failed to load bookmarks file: %1")).arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

				QMessageBox::warning(nullptr, tr("Error"), (isNotes ? tr("Failed to load notes file.") : tr("Failed to load bookmarks file.")), QMessageBox::Close);

				return;
			}
		}
	}

	connect(this, &BookmarksModel::itemChanged, this, &BookmarksModel::modelModified);
	connect(this, &BookmarksModel::rowsInserted, this, &BookmarksModel::modelModified);
	connect(this, &BookmarksModel::rowsInserted, this, &BookmarksModel::notifyBookmarkModified);
	connect(this, &BookmarksModel::rowsRemoved, this, &BookmarksModel::modelModified);
	connect(this, &BookmarksModel::rowsRemoved, this, &BookmarksModel::notifyBookmarkModified);
	connect(this, &BookmarksModel::rowsMoved, this, &BookmarksModel::modelModified);
}

void BookmarksModel::beginImport(Bookmark *target, int estimatedUrlsAmount, int estimatedKeywordsAmount)
{
	m_importTargetItem = target;

	beginResetModel();
	blockSignals(true);

	if (estimatedUrlsAmount > 0)
	{
		m_urls.reserve(m_urls.count() + estimatedUrlsAmount);
	}

	if (estimatedKeywordsAmount > 0)
	{
		m_keywords.reserve(m_keywords.count() + estimatedKeywordsAmount);
	}
}

void BookmarksModel::endImport()
{
	m_urls.squeeze();
	m_keywords.squeeze();

	blockSignals(false);
	endResetModel();

	if (m_importTargetItem)
	{
		emit bookmarkModified(m_importTargetItem);

		m_importTargetItem = nullptr;
	}

	emit modelModified();
}

void BookmarksModel::trashBookmark(Bookmark *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	const BookmarkType type(bookmark->getType());

	if (type == RootBookmark || type == TrashBookmark)
	{
		return;
	}

	if (type == SeparatorBookmark || bookmark->data(IsTrashedRole).toBool())
	{
		bookmark->remove();

		return;
	}

	Bookmark *previousParent(bookmark->getParent());
	BookmarkLocation location;
	location.parent = bookmark->parent()->index();
	location.row = bookmark->row();

	m_trash[bookmark] = location;

	m_trashItem->appendRow(bookmark->parent()->takeRow(bookmark->row()));
	m_trashItem->setEnabled(true);

	removeBookmarkUrl(bookmark);

	emit bookmarkModified(bookmark);
	emit bookmarkTrashed(bookmark, previousParent);
	emit modelModified();
}

void BookmarksModel::restoreBookmark(Bookmark *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	Bookmark *previousParent(m_trash.contains(bookmark) ? getBookmark(m_trash[bookmark].parent) : m_rootItem);

	if (!previousParent || previousParent->getType() != FolderBookmark)
	{
		previousParent = m_rootItem;
	}

	if (m_trash.contains(bookmark))
	{
		previousParent->insertRow(m_trash[bookmark].row, bookmark->parent()->takeRow(bookmark->row()));

		m_trash.remove(bookmark);
	}
	else
	{
		previousParent->appendRow(bookmark->parent()->takeRow(bookmark->row()));
	}

	readdBookmarkUrl(bookmark);

	m_trashItem->setEnabled(m_trashItem->hasChildren());

	emit bookmarkModified(bookmark);
	emit bookmarkRestored(bookmark);
	emit modelModified();
}

void BookmarksModel::removeBookmark(Bookmark *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	removeBookmarkUrl(bookmark);

	const quint64 identifier(bookmark->data(IdentifierRole).toULongLong());

	if (identifier > 0 && m_identifiers.contains(identifier))
	{
		m_identifiers.remove(identifier);
	}

	if (!bookmark->data(KeywordRole).toString().isEmpty() && m_keywords.contains(bookmark->data(KeywordRole).toString()))
	{
		m_keywords.remove(bookmark->data(KeywordRole).toString());
	}

	emit bookmarkRemoved(bookmark, bookmark->getParent());

	bookmark->parent()->removeRow(bookmark->row());

	emit modelModified();
}

void BookmarksModel::readBookmark(QXmlStreamReader *reader, Bookmark *parent)
{
	Bookmark *bookmark(nullptr);

	if (reader->name() == QLatin1String("folder"))
	{
		bookmark = addBookmark(FolderBookmark, {{IdentifierRole, reader->attributes().value(QLatin1String("id")).toULongLong()}, {TimeAddedRole, readDateTime(reader, QLatin1String("added"))}, {TimeModifiedRole, readDateTime(reader, QLatin1String("modified"))}}, parent);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setItemData(reader->readElementText().trimmed(), TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setItemData(reader->readElementText().trimmed(), DescriptionRole);
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
							if (reader->name() == QLatin1String("metadata") && reader->attributes().value(QLatin1String("owner")).toString().startsWith(QLatin1String("http://otter-browser.org/")))
							{
								while (reader->readNext())
								{
									if (reader->isStartElement())
									{
										if (reader->name() == QLatin1String("keyword"))
										{
											const QString keyword(reader->readElementText().trimmed());

											if (!keyword.isEmpty())
											{
												bookmark->setItemData(keyword, KeywordRole);

												handleKeywordChanged(bookmark, keyword);
											}
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
		const bool isFeed(reader->attributes().hasAttribute(QLatin1String("feed")));

		bookmark = addBookmark((isFeed ? FeedBookmark : UrlBookmark), {{IdentifierRole, reader->attributes().value(QLatin1String("id")).toULongLong()}, {UrlRole, reader->attributes().value(QLatin1String("href")).toString()}, {TimeAddedRole, readDateTime(reader, QLatin1String("added"))}, {TimeModifiedRole, readDateTime(reader, QLatin1String("modified"))}, {TimeVisitedRole, readDateTime(reader, QLatin1String("visited"))}}, parent);

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == QLatin1String("title"))
				{
					bookmark->setItemData(reader->readElementText().trimmed(), TitleRole);
				}
				else if (reader->name() == QLatin1String("desc"))
				{
					bookmark->setItemData(reader->readElementText().trimmed(), DescriptionRole);
				}
				else if (reader->name() == QLatin1String("info"))
				{
					while (reader->readNext())
					{
						if (reader->isStartElement())
						{
							if (reader->name() == QLatin1String("metadata") && reader->attributes().value(QLatin1String("owner")).toString().startsWith(QLatin1String("http://otter-browser.org/")))
							{
								while (reader->readNext())
								{
									if (reader->isStartElement())
									{
										if (reader->name() == QLatin1String("keyword"))
										{
											const QString keyword(reader->readElementText().trimmed());

											if (!keyword.isEmpty())
											{
												bookmark->setItemData(keyword, KeywordRole);

												handleKeywordChanged(bookmark, keyword);
											}
										}
										else if (reader->name() == QLatin1String("visits"))
										{
											bookmark->setItemData(reader->readElementText().toInt(), VisitsRole);
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

		if (isFeed)
		{
			setupFeed(bookmark);
		}
	}
	else if (reader->name() == QLatin1String("separator"))
	{
		addBookmark(SeparatorBookmark, {}, parent);

		reader->readNext();
	}
}

void BookmarksModel::writeBookmark(QXmlStreamWriter *writer, Bookmark *bookmark) const
{
	if (!bookmark)
	{
		return;
	}

	const QString elementOwner(QLatin1String("http://otter-browser.org/otter-xbel-bookmark"));
	const BookmarkType type(bookmark->getType());
	const bool isBookmarks(m_mode == BookmarksMode);

	switch (type)
	{
		case FeedBookmark:
		case UrlBookmark:
			writer->writeStartElement(QLatin1String("bookmark"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->getRawData(IdentifierRole).toULongLong()));

			if (type == FeedBookmark)
			{
				writer->writeAttribute(QLatin1String("feed"), QLatin1String("true"));
			}

			if (!bookmark->getRawData(UrlRole).toString().isEmpty())
			{
				writer->writeAttribute(QLatin1String("href"), bookmark->getRawData(UrlRole).toString());
			}

			if (bookmark->getRawData(TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->getRawData(TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->getRawData(TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->getRawData(TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			if (isBookmarks)
			{
				if (bookmark->getRawData(TimeVisitedRole).toDateTime().isValid())
				{
					writer->writeAttribute(QLatin1String("visited"), bookmark->getRawData(TimeVisitedRole).toDateTime().toString(Qt::ISODate));
				}

				writer->writeTextElement(QLatin1String("title"), bookmark->getRawData(TitleRole).toString());
			}

			if (!bookmark->getRawData(DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->getRawData(DescriptionRole).toString());
			}

			if (isBookmarks && (!bookmark->getRawData(KeywordRole).toString().isEmpty() || bookmark->getRawData(VisitsRole).toInt() > 0))
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), elementOwner);

				if (!bookmark->getRawData(KeywordRole).toString().isEmpty())
				{
					writer->writeTextElement(QLatin1String("keyword"), bookmark->getRawData(KeywordRole).toString());
				}

				if (bookmark->getRawData(VisitsRole).toInt() > 0)
				{
					writer->writeTextElement(QLatin1String("visits"), QString::number(bookmark->getRawData(VisitsRole).toInt()));
				}

				writer->writeEndElement();
				writer->writeEndElement();
			}

			writer->writeEndElement();

			break;
		case FolderBookmark:
			writer->writeStartElement(QLatin1String("folder"));
			writer->writeAttribute(QLatin1String("id"), QString::number(bookmark->getRawData(IdentifierRole).toULongLong()));

			if (bookmark->getRawData(TimeAddedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("added"), bookmark->getRawData(TimeAddedRole).toDateTime().toString(Qt::ISODate));
			}

			if (bookmark->getRawData(TimeModifiedRole).toDateTime().isValid())
			{
				writer->writeAttribute(QLatin1String("modified"), bookmark->getRawData(TimeModifiedRole).toDateTime().toString(Qt::ISODate));
			}

			writer->writeTextElement(QLatin1String("title"), bookmark->getRawData(TitleRole).toString());

			if (!bookmark->getRawData(DescriptionRole).toString().isEmpty())
			{
				writer->writeTextElement(QLatin1String("desc"), bookmark->getRawData(DescriptionRole).toString());
			}

			if (isBookmarks && !bookmark->getRawData(KeywordRole).toString().isEmpty())
			{
				writer->writeStartElement(QLatin1String("info"));
				writer->writeStartElement(QLatin1String("metadata"));
				writer->writeAttribute(QLatin1String("owner"), elementOwner);
				writer->writeTextElement(QLatin1String("keyword"), bookmark->getRawData(KeywordRole).toString());
				writer->writeEndElement();
				writer->writeEndElement();
			}

			for (int i = 0; i < bookmark->rowCount(); ++i)
			{
				writeBookmark(writer, bookmark->getChild(i));
			}

			writer->writeEndElement();

			break;
		default:
			writer->writeEmptyElement(QLatin1String("separator"));

			break;
	}
}

void BookmarksModel::removeBookmarkUrl(Bookmark *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	switch (bookmark->getType())
	{
		case FeedBookmark:
		case UrlBookmark:
			{
				const QUrl url(Utils::normalizeUrl(bookmark->data(UrlRole).toUrl()));

				if (!url.isEmpty() && m_urls.contains(url))
				{
					m_urls[url].removeAll(bookmark);

					if (m_urls[url].isEmpty())
					{
						m_urls.remove(url);
					}
				}
			}

			break;
		case FolderBookmark:
			for (int i = 0; i < bookmark->rowCount(); ++i)
			{
				removeBookmarkUrl(bookmark->getChild(i));
			}

			break;
		default:
			break;
	}
}

void BookmarksModel::readdBookmarkUrl(Bookmark *bookmark)
{
	if (!bookmark)
	{
		return;
	}

	switch (bookmark->getType())
	{
		case FeedBookmark:
		case UrlBookmark:
			{
				const QUrl url(Utils::normalizeUrl(bookmark->data(UrlRole).toUrl()));

				if (!url.isEmpty())
				{
					if (!m_urls.contains(url))
					{
						m_urls[url] = {};
					}

					m_urls[url].append(bookmark);
				}
			}

			break;
		case FolderBookmark:
			for (int i = 0; i < bookmark->rowCount(); ++i)
			{
				readdBookmarkUrl(bookmark->getChild(i));
			}

			break;
		default:
			break;
	}
}

void BookmarksModel::setupFeed(Bookmark *bookmark)
{
	const QUrl normalizedUrl(Utils::normalizeUrl(bookmark->getUrl()));
	Feed *feed(FeedsManager::createFeed(bookmark->getUrl(), bookmark->getTitle()));

	if (!m_feeds.contains(normalizedUrl))
	{
		m_feeds[normalizedUrl] = {};

		connect(feed, &Feed::entriesModified, this, &BookmarksModel::handleFeedModified);
	}

	m_feeds[normalizedUrl].append(bookmark);

	if (feed)
	{
		handleFeedModified(feed);
	}
}

void BookmarksModel::emptyTrash()
{
	if (m_trashItem->hasChildren())
	{
		m_trashItem->removeRows(0, m_trashItem->rowCount());
		m_trashItem->setEnabled(false);

		m_trash.clear();

		emit modelModified();
	}
}

void BookmarksModel::handleFeedModified(Feed *feed)
{
	if (!hasFeed(feed->getUrl()))
	{
		return;
	}

	const QUrl normalizedUrl(Utils::normalizeUrl(feed->getUrl()));
	QVector<Bookmark*> bookmarks(m_feeds.value(feed->getUrl()));

	if (feed->getUrl() != normalizedUrl)
	{
		bookmarks.append(m_feeds.value(normalizedUrl));
	}

	if (bookmarks.isEmpty())
	{
		return;
	}

	beginResetModel();
	blockSignals(true);

	for (Bookmark *bookmark: bookmarks)
	{
		bookmark->removeRows(0, bookmark->rowCount());

		const QVector<Feed::Entry> entries(feed->getEntries());

		for (const Feed::Entry &entry: entries)
		{
			if (entry.url.isValid())
			{
				addBookmark(UrlBookmark, {{UrlRole, entry.url}, {TitleRole, entry.title}, {DescriptionRole, entry.summary}}, bookmark);
			}
		}
	}

	blockSignals(false);
	endResetModel();

    for (Bookmark *bookmark: bookmarks)
	{
		emit bookmarkModified(bookmark);
	}

	emit modelModified();
}

void BookmarksModel::handleKeywordChanged(Bookmark *bookmark, const QString &newKeyword, const QString &oldKeyword)
{
	if (!oldKeyword.isEmpty() && m_keywords.contains(oldKeyword))
	{
		m_keywords.remove(oldKeyword);
	}

	if (!newKeyword.isEmpty())
	{
		m_keywords[newKeyword] = bookmark;
	}
}

void BookmarksModel::handleUrlChanged(Bookmark *bookmark, const QUrl &newUrl, const QUrl &oldUrl)
{
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
			m_urls[newUrl] = {};
		}

		m_urls[newUrl].append(bookmark);
	}
}

void BookmarksModel::notifyBookmarkModified(const QModelIndex &index)
{
	Bookmark *bookmark(getBookmark(index));

	if (bookmark)
	{
		emit bookmarkModified(bookmark);
	}
}

BookmarksModel::Bookmark* BookmarksModel::addBookmark(BookmarkType type, const QMap<int, QVariant> &metaData, Bookmark *parent, int index)
{
	Bookmark *bookmark(new Bookmark());

	if (!parent)
	{
		parent = m_rootItem;
	}

	parent->insertRow(((index < 0) ? parent->rowCount() : index), bookmark);

	if (type == FeedBookmark || type == UrlBookmark || type == SeparatorBookmark)
	{
		bookmark->setDropEnabled(false);
	}

	if (type == FeedBookmark || type == FolderBookmark || type == UrlBookmark)
	{
		quint64 identifier(metaData.value(IdentifierRole).toULongLong());

		if (identifier == 0 || m_identifiers.contains(identifier))
		{
			identifier = (m_identifiers.isEmpty() ? 1 : (m_identifiers.lastKey() + 1));
		}

		m_identifiers[identifier] = bookmark;

		setItemData(bookmark->index(), metaData);

		bookmark->setItemData(identifier, IdentifierRole);

		if (!metaData.contains(TimeAddedRole) || !metaData.contains(TimeModifiedRole))
		{
			const QDateTime currentDateTime(QDateTime::currentDateTimeUtc());

			bookmark->setItemData(currentDateTime, TimeAddedRole);
			bookmark->setItemData(currentDateTime, TimeModifiedRole);
		}

		if (type == FeedBookmark || type == UrlBookmark)
		{
			const QUrl url(metaData.value(UrlRole).toUrl());

			if (!url.isEmpty())
			{
				handleUrlChanged(bookmark, Utils::normalizeUrl(url));
			}

			if (type == UrlBookmark)
			{
				bookmark->setFlags(bookmark->flags() | Qt::ItemNeverHasChildren);
			}
		}
	}

	if (type == FeedBookmark)
	{
		setupFeed(bookmark);
	}

	bookmark->setItemData(type, TypeRole);

	emit bookmarkAdded(bookmark);
	emit modelModified();

	return bookmark;
}

BookmarksModel::Bookmark* BookmarksModel::getBookmarkByKeyword(const QString &keyword) const
{
	if (m_keywords.contains(keyword))
	{
		return m_keywords[keyword];
	}

	return nullptr;
}

BookmarksModel::Bookmark* BookmarksModel::getBookmarkByPath(const QString &path, bool createIfNotExists)
{
	if (path == QLatin1String("/"))
	{
		return m_rootItem;
	}

	if (path.startsWith(QLatin1Char('#')))
	{
		return getBookmark(path.mid(1).toULongLong());
	}

	Bookmark *bookmark(m_rootItem);
	const QStringList directories(path.split(QLatin1Char('/'), Qt::SkipEmptyParts));

	for (const QString &directory: directories)
	{
		bool hasMatch(false);

		for (int i = 0; i < bookmark->rowCount(); ++i)
		{
			Bookmark *childBookmark(bookmark->getChild(i));

			if (childBookmark && childBookmark->getTitle() == directory)
			{
				bookmark = childBookmark;

				hasMatch = true;

				break;
			}
		}

		if (!hasMatch)
		{
			if (!createIfNotExists)
			{
				return nullptr;
			}

			bookmark = addBookmark(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, directory}}, bookmark);
		}
	}

	return bookmark;
}

BookmarksModel::Bookmark* BookmarksModel::getBookmark(const QModelIndex &index) const
{
	Bookmark *bookmark(static_cast<Bookmark*>(itemFromIndex(index)));

	if (bookmark)
	{
		return bookmark;
	}

	const QVariant data(index.data(BookmarksModel::IdentifierRole));

	return (data.isValid() ? getBookmark(data.toULongLong()) : nullptr);
}

BookmarksModel::Bookmark* BookmarksModel::getBookmark(quint64 identifier) const
{
	if (identifier == 0)
	{
		return m_rootItem;
	}

	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	return nullptr;
}

BookmarksModel::Bookmark* BookmarksModel::getRootItem() const
{
	return m_rootItem;
}

BookmarksModel::Bookmark* BookmarksModel::getTrashItem() const
{
	return m_trashItem;
}

QMimeData* BookmarksModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData(new QMimeData());
	QStringList texts;
	texts.reserve(indexes.count());

	QList<QUrl> urls;
	urls.reserve(indexes.count());

	if (indexes.count() == 1)
	{
		const QModelIndex index(indexes.at(0));

		mimeData->setProperty("x-item-index", index);
		mimeData->setProperty("x-url-title", index.data(TitleRole).toString());
	}

	for (const QModelIndex &index: indexes)
	{
		if (index.isValid() && static_cast<BookmarkType>(index.data(TypeRole).toInt()) == UrlBookmark)
		{
			texts.append(index.data(UrlRole).toString());
			urls.append(index.data(UrlRole).toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	return mimeData;
}

QDateTime BookmarksModel::readDateTime(QXmlStreamReader *reader, const QString &attribute)
{
	QDateTime dateTime(QDateTime::fromString(reader->attributes().value(attribute).toString(), Qt::ISODate));
	dateTime.setTimeZone(QTimeZone::utc());

	return dateTime;
}

QStringList BookmarksModel::mimeTypes() const
{
	return {QLatin1String("text/uri-list")};
}

QStringList BookmarksModel::getKeywords() const
{
	return m_keywords.keys();
}

QVector<BookmarksModel::BookmarkMatch> BookmarksModel::findBookmarks(const QString &prefix) const
{
	QVector<Bookmark*> matchedBookmarks;
	QVector<BookmarkMatch> allMatches;
	QVector<BookmarkMatch> currentMatches;
	QMultiMap<QDateTime, BookmarkMatch> matchesMap;
	QHash<QString, Bookmark*>::const_iterator keywordsIterator;

	for (keywordsIterator = m_keywords.constBegin(); keywordsIterator != m_keywords.constEnd(); ++keywordsIterator)
	{
		if (!keywordsIterator.key().startsWith(prefix, Qt::CaseInsensitive))
		{
			continue;
		}

		BookmarkMatch match;
		match.bookmark = keywordsIterator.value();
		match.match = keywordsIterator.key();

		matchesMap.insert(match.bookmark->getTimeVisited(), match);

		matchedBookmarks.append(match.bookmark);
	}

	currentMatches = matchesMap.values().toVector();

	matchesMap.clear();

	allMatches.reserve(currentMatches.count());

	for (int i = (currentMatches.count() - 1); i >= 0; --i)
	{
		allMatches.append(currentMatches.at(i));
	}

	QHash<QUrl, QVector<Bookmark*> >::const_iterator urlsIterator;

	for (urlsIterator = m_urls.constBegin(); urlsIterator != m_urls.constEnd(); ++urlsIterator)
	{
		if (urlsIterator.value().isEmpty() || matchedBookmarks.contains(urlsIterator.value().value(0)))
		{
			continue;
		}

		const QString result(Utils::matchUrl(urlsIterator.key(), prefix));

		if (!result.isEmpty())
		{
			BookmarkMatch match;
			match.bookmark = urlsIterator.value().value(0);
			match.match = result;

			matchesMap.insert(match.bookmark->getTimeVisited(), match);

			matchedBookmarks.append(match.bookmark);
		}
	}

	currentMatches = matchesMap.values().toVector();

	matchesMap.clear();

	for (int i = (currentMatches.count() - 1); i >= 0; --i)
	{
		allMatches.append(currentMatches.at(i));
	}

	return allMatches;
}

QVector<BookmarksModel::Bookmark*> BookmarksModel::findUrls(const QUrl &url, Bookmark *branch) const
{
	if (!branch)
	{
		branch = m_rootItem;
	}

	const QUrl normalizedUrl(Utils::normalizeUrl(url));
	QVector<Bookmark*> bookmarks;

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		Bookmark *bookmark(branch->getChild(i));

		if (!bookmark)
		{
			continue;
		}

		switch (bookmark->getType())
		{
			case FolderBookmark:
				bookmarks.append(findUrls(url, bookmark));

				break;
			case UrlBookmark:
				if (normalizedUrl == Utils::normalizeUrl(bookmark->getUrl()))
				{
					bookmarks.append(bookmark);
				}

				break;
			default:
				break;
		}
	}

	return bookmarks;
}

QVector<BookmarksModel::Bookmark*> BookmarksModel::getBookmarks(const QUrl &url) const
{
	const QUrl normalizedUrl(Utils::normalizeUrl(url));
	QVector<BookmarksModel::Bookmark*> bookmarks;

	if (m_urls.contains(url))
	{
		bookmarks = m_urls[url];
	}

	if (url != normalizedUrl && m_urls.contains(normalizedUrl))
	{
		bookmarks.append(m_urls[normalizedUrl]);
	}

	return bookmarks;
}

BookmarksModel::FormatMode BookmarksModel::getFormatMode() const
{
	return m_mode;
}

int BookmarksModel::getCount() const
{
	return m_identifiers.count();
}

bool BookmarksModel::moveBookmark(Bookmark *bookmark, Bookmark *newParent, int newRow)
{
	if (!bookmark || !newParent || bookmark == newParent || bookmark->isAncestorOf(newParent))
	{
		return false;
	}

	Bookmark *previousParent(bookmark->getParent());

	if (!previousParent)
	{
		if (newRow < 0)
		{
			newParent->appendRow(bookmark);
		}
		else
		{
			newParent->insertRow(newRow, bookmark);
		}

		emit modelModified();

		return true;
	}

	const int previousRow(bookmark->row());

	if (newRow < 0)
	{
		newParent->appendRow(bookmark->parent()->takeRow(bookmark->row()));

		emit bookmarkMoved(bookmark, previousParent, previousRow);
		emit modelModified();

		return true;
	}

	int targetRow(newRow);

	if (bookmark->parent() == newParent && bookmark->row() < newRow)
	{
		--targetRow;
	}

	newParent->insertRow(targetRow, bookmark->parent()->takeRow(bookmark->row()));

	emit bookmarkMoved(bookmark, previousParent, previousRow);
	emit modelModified();

	return true;
}

bool BookmarksModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (index.isValid())
	{
		const Bookmark *bookmark(getBookmark(index));
		Bookmark *newParent(getBookmark(parent));

		return (bookmark && newParent && bookmark != newParent && !bookmark->isAncestorOf(newParent));
	}

	return QStandardItemModel::canDropMimeData(data, action, row, column, parent);
}

bool BookmarksModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (!Bookmark::isFolder(static_cast<BookmarkType>(parent.data(TypeRole).toInt())))
	{
		return false;
	}

	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (index.isValid())
	{
		return moveBookmark(getBookmark(index), getBookmark(parent), row);
	}

	if (data->hasUrls())
	{
		const QVector<QUrl> urls(Utils::extractUrls(data));

		for (const QUrl &url: urls)
		{
			addBookmark(UrlBookmark, {{UrlRole, url}, {TitleRole, (data->property("x-url-title").toString().isEmpty() ? url.toString() : data->property("x-url-title").toString())}}, getBookmark(parent), row);
		}

		return true;
	}

	return QStandardItemModel::dropMimeData(data, action, row, column, parent);
}

bool BookmarksModel::save(const QString &path) const
{
	if (SessionsManager::isReadOnly())
	{
		return false;
	}

	QSaveFile file(path);

	if (!file.open(QIODevice::WriteOnly))
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

	for (int i = 0; i < m_rootItem->rowCount(); ++i)
	{
		writeBookmark(&writer, m_rootItem->getChild(i));
	}

	writer.writeEndDocument();

	return file.commit();
}

bool BookmarksModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Bookmark *bookmark(getBookmark(index));

	if (!bookmark)
	{
		return QStandardItemModel::setData(index, value, role);
	}

	switch (role)
	{
		case DescriptionRole:
			if (m_mode == NotesMode)
			{
				const QString title(value.toString().section(QLatin1Char('\n'), 0, 0).left(100));

				setData(index, ((title == value.toString().trimmed()) ? title : title + QStringLiteral("…")), TitleRole);
			}

			break;
		case KeywordRole:
			{
				const QString oldKeyword(index.data(KeywordRole).toString());
				const QString newKeyword(value.toString());

				if (newKeyword != oldKeyword)
				{
					handleKeywordChanged(bookmark, newKeyword, oldKeyword);
				}
			}

			break;
		case UrlRole:
			{
				const QUrl oldUrl(Utils::normalizeUrl(index.data(UrlRole).toUrl()));
				const QUrl newUrl(Utils::normalizeUrl(value.toUrl()));

				if (oldUrl != newUrl)
				{
					handleUrlChanged(bookmark, newUrl, oldUrl);
				}
			}

			break;
		default:
			break;
	}

	bookmark->setItemData(value, role);

	switch (role)
	{
		case TitleRole:
		case UrlRole:
		case DescriptionRole:
		case IdentifierRole:
		case TypeRole:
		case KeywordRole:
		case TimeAddedRole:
		case TimeModifiedRole:
		case TimeVisitedRole:
		case VisitsRole:
			emit bookmarkModified(bookmark);
			emit modelModified();

			break;
		default:
			break;
	}

	return true;
}

bool BookmarksModel::hasBookmark(const QUrl &url) const
{
	return (m_urls.contains(url) || m_urls.contains(Utils::normalizeUrl(url)));
}

bool BookmarksModel::hasFeed(const QUrl &url) const
{
	return (m_feeds.contains(url) || m_feeds.contains(Utils::normalizeUrl(url)));
}

bool BookmarksModel::hasKeyword(const QString &keyword) const
{
	return m_keywords.contains(keyword);
}

}
