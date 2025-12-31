/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "FeedsModel.h"
#include "Application.h"
#include "Console.h"
#include "FeedsManager.h"
#include "SessionsManager.h"
#include "ThemesManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtWidgets/QMessageBox>

namespace Otter
{

FeedsModel::Entry::Entry(Feed *feed) :
	m_feed(feed)
{
}

FeedsModel::Entry* FeedsModel::Entry::getChild(int index) const
{
	FeedsModel *model(qobject_cast<FeedsModel*>(this->model()));

	if (model)
	{
		return model->getEntry(model->index(index, 0, this->index()));
	}

	return nullptr;
}

Feed* FeedsModel::Entry::getFeed() const
{
	return m_feed;
}

QString FeedsModel::Entry::getTitle() const
{
	return data(TitleRole).toString();
}

QVariant FeedsModel::Entry::data(int role) const
{
	if (role == TitleRole)
	{
		switch (getType())
		{
			case RootEntry:
				return QCoreApplication::translate("Otter::FeedsModel", "Feeds");
			case TrashEntry:
				return QCoreApplication::translate("Otter::FeedsModel", "Trash");
			case FolderEntry:
				if (QStandardItem::data(role).isNull())
				{
					return QCoreApplication::translate("Otter::FeedsModel", "(Untitled)");
				}

				break;
			case FeedEntry:
				if (m_feed && !m_feed->getTitle().isEmpty())
				{
					return m_feed->getTitle();
				}

				break;
			default:
				break;
		}
	}

	if (role == Qt::DecorationRole)
	{
		switch (getType())
		{
			case RootEntry:
			case FolderEntry:
				return ThemesManager::createIcon(QLatin1String("inode-directory"));
			case TrashEntry:
				return ThemesManager::createIcon(QLatin1String("user-trash"));
			case FeedEntry:
				if (m_feed)
				{
					if (m_feed->getError() != Feed::NoError)
					{
						return ThemesManager::createIcon(QLatin1String("dialog-error"));
					}

					const QIcon icon(m_feed->getIcon());

					if (!icon.isNull())
					{
						return icon;
					}
				}

				return ThemesManager::createIcon(QLatin1String("application-rss+xml"));
			default:
				break;
		}

		return {};
	}

	if (m_feed)
	{
		switch (role)
		{
			case LastUpdateTimeRole:
				return m_feed->getLastUpdateTime();
			case LastSynchronizationTimeRole:
				return m_feed->getLastSynchronizationTime();
			case UnreadEntriesAmountRole:
				return m_feed->getUnreadEntriesAmount();
			case UrlRole:
				return m_feed->getUrl();
			case UpdateIntervalRole:
				return m_feed->getUpdateInterval();
			case UpdateProgressValueRole:
				return m_feed->getUpdateProgress();
			case IsShowingProgressIndicatorRole:
				return (m_feed->isUpdating() || (m_feed->getLastSynchronizationTime().isValid() && m_feed->getLastSynchronizationTime().msecsTo(QDateTime::currentDateTimeUtc()) < 2500));
			case IsUpdatingRole:
				return m_feed->isUpdating();
			case HasErrorRole:
				return (m_feed->getError() != Feed::NoError);
			default:
				break;
		}
	}

	if (role == IsTrashedRole)
	{
		QModelIndex parent(index().parent());

		while (parent.isValid())
		{
			const EntryType type(static_cast<EntryType>(parent.data(TypeRole).toInt()));

			if (type == RootEntry)
			{
				break;
			}

			if (type == TrashEntry)
			{
				return true;
			}

			parent = parent.parent();
		}

		return false;
	}

	return QStandardItem::data(role);
}

QVariant FeedsModel::Entry::getRawData(int role) const
{
	if (m_feed)
	{
		switch (role)
		{
			case Qt::DecorationRole:
				return m_feed->getIcon();
			case TitleRole:
				return m_feed->getTitle();
			case LastUpdateTimeRole:
				return m_feed->getLastUpdateTime();
			case LastSynchronizationTimeRole:
				return m_feed->getLastSynchronizationTime();
			case UrlRole:
				return m_feed->getUrl();
			case UpdateIntervalRole:
				return m_feed->getUpdateInterval();
			default:
				break;
		}
	}

	return QStandardItem::data(role);
}

QVector<Feed*> FeedsModel::Entry::getFeeds() const
{
	QVector<Feed*> feeds;

	if (getType() == FeedEntry)
	{
		feeds.append(m_feed);
	}

	for (int i = 0; i < rowCount(); ++i)
	{
		const Entry *entry(getChild(i));

		if (!entry)
		{
			continue;
		}

		switch (entry->getType())
		{
			case FeedEntry:
				feeds.append(entry->getFeed());

				break;
			case FolderEntry:
				feeds.append(entry->getFeeds());

				break;
			default:
				break;
		}
	}

	return feeds;
}

quint64 FeedsModel::Entry::getIdentifier() const
{
	return QStandardItem::data(IdentifierRole).toULongLong();
}

FeedsModel::EntryType FeedsModel::Entry::getType() const
{
	return static_cast<EntryType>(data(TypeRole).toInt());
}

bool FeedsModel::Entry::isFolder(FeedsModel::EntryType type)
{
	switch (type)
	{
		case RootEntry:
		case TrashEntry:
		case FolderEntry:
			return true;
		default:
			return false;
	}

	return false;
}

bool FeedsModel::Entry::isFolder() const
{
	return isFolder(getType());
}

bool FeedsModel::Entry::isAncestorOf(FeedsModel::Entry *child) const
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

bool FeedsModel::Entry::operator<(const QStandardItem &other) const
{
	const EntryType type(getType());

	if (type == RootEntry || type == TrashEntry)
	{
		return false;
	}

	return QStandardItem::operator<(other);
}

FeedsModel::FeedsModel(const QString &path, QObject *parent) : QStandardItemModel(parent),
	m_rootEntry(new Entry()),
	m_trashEntry(new Entry()),
	m_importTargetEntry(nullptr)
{
	m_rootEntry->setData(RootEntry, TypeRole);
	m_rootEntry->setDragEnabled(false);
	m_trashEntry->setData(TrashEntry, TypeRole);
	m_trashEntry->setDragEnabled(false);
	m_trashEntry->setEnabled(false);

	appendRow(m_rootEntry);
	appendRow(m_trashEntry);
	setItemPrototype(new Entry());

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		Console::addMessage(tr("Failed to open feeds file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

		return;
	}

	QXmlStreamReader reader(&file);

	if (reader.readNextStartElement() && reader.name() == QLatin1String("opml") && reader.attributes().value(QLatin1String("version")).toString() == QLatin1String("1.0"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("outline"))
			{
				readEntry(&reader, m_rootEntry);
			}

			if (reader.name() != QLatin1String("body"))
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError() && rowCount() == 0)
			{
				Console::addMessage(tr("Failed to load feeds file: %1").arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

				QMessageBox::warning(nullptr, tr("Error"), tr("Failed to load feeds file."), QMessageBox::Close);

				return;
			}
		}
	}

	file.close();
}

void FeedsModel::beginImport(Entry *target, int estimatedUrlsAmount)
{
	m_importTargetEntry = target;

	beginResetModel();
	blockSignals(true);

	if (estimatedUrlsAmount > 0)
	{
		m_urls.reserve(m_urls.count() + estimatedUrlsAmount);
	}
}

void FeedsModel::endImport()
{
	m_urls.squeeze();

	blockSignals(false);
	endResetModel();

	if (m_importTargetEntry)
	{
		emit entryModified(m_importTargetEntry);

		m_importTargetEntry = nullptr;
	}

	emit modelModified();
}

void FeedsModel::trashEntry(Entry *entry)
{
	if (!entry)
	{
		return;
	}

	const EntryType type(entry->getType());

	if (type == RootEntry || type == TrashEntry)
	{
		return;
	}

	if (entry->data(IsTrashedRole).toBool())
	{
		removeEntry(entry);

		return;
	}

	Entry *previousParent(static_cast<Entry*>(entry->parent()));
	EntryLocation location;
	location.parent = entry->parent()->index();
	location.row = entry->row();

	m_trash[entry] = location;

	m_trashEntry->appendRow(entry->parent()->takeRow(entry->row()));
	m_trashEntry->setEnabled(true);

	removeEntryUrl(entry);

	emit entryModified(entry);
	emit entryTrashed(entry, previousParent);
	emit modelModified();
}

void FeedsModel::restoreEntry(Entry *entry)
{
	if (!entry)
	{
		return;
	}

	Entry *previousParent(m_trash.contains(entry) ? getEntry(m_trash[entry].parent) : m_rootEntry);

	if (!previousParent || previousParent->getType() != FolderEntry)
	{
		previousParent = m_rootEntry;
	}

	if (m_trash.contains(entry))
	{
		previousParent->insertRow(m_trash[entry].row, entry->parent()->takeRow(entry->row()));

		m_trash.remove(entry);
	}
	else
	{
		previousParent->appendRow(entry->parent()->takeRow(entry->row()));
	}

	readdEntryUrl(entry);

	m_trashEntry->setEnabled(m_trashEntry->rowCount() > 0);

	emit entryModified(entry);
	emit entryRestored(entry);
	emit modelModified();
}

void FeedsModel::removeEntry(Entry *entry)
{
	if (!entry)
	{
		return;
	}

	removeEntryUrl(entry);

	const quint64 identifier(entry->data(IdentifierRole).toULongLong());

	if (identifier > 0 && m_identifiers.contains(identifier))
	{
		m_identifiers.remove(identifier);
	}

	emit entryRemoved(entry, static_cast<Entry*>(entry->parent()));

	entry->parent()->removeRow(entry->row());

	emit modelModified();
}

void FeedsModel::readEntry(QXmlStreamReader *reader, Entry *parent)
{
	const QString title(reader->attributes().value(reader->attributes().hasAttribute(QLatin1String("title")) ? QLatin1String("title") : QLatin1String("text")).toString());

	if (reader->attributes().hasAttribute(QLatin1String("xmlUrl")))
	{
		const QUrl url(Utils::normalizeUrl(QUrl(reader->attributes().value(QLatin1String("xmlUrl")).toString())));

		if (url.isValid())
		{
			Entry *entry(new Entry(FeedsManager::createFeed(url, title, Utils::loadPixmapFromDataUri(reader->attributes().value(QLatin1String("icon")).toString()), reader->attributes().value(QLatin1String("updateInterval")).toInt())));
			entry->setData(FeedEntry, TypeRole);
			entry->setFlags(entry->flags() | Qt::ItemNeverHasChildren);

			createIdentifier(entry);
			handleUrlChanged(entry, url);

			parent->appendRow(entry);
		}

		return;
	}

	Entry *entry(new Entry());
	entry->setData(FolderEntry, TypeRole);
	entry->setData(title, TitleRole);

	createIdentifier(entry);

	parent->appendRow(entry);

	while (reader->readNext())
	{
		if (reader->isStartElement())
		{
			if (reader->name() == QLatin1String("outline"))
			{
				readEntry(reader, entry);
			}
			else
			{
				reader->skipCurrentElement();
			}
		}
		else if (reader->hasError())
		{
			return;
		}
	}
}

void FeedsModel::writeEntry(QXmlStreamWriter *writer, Entry *entry) const
{
	if (!entry)
	{
		return;
	}

	writer->writeStartElement(QLatin1String("outline"));
	writer->writeAttribute(QLatin1String("text"), entry->getRawData(TitleRole).toString());
	writer->writeAttribute(QLatin1String("title"), entry->getRawData(TitleRole).toString());

	switch (entry->getType())
	{
		case FolderEntry:
			for (int i = 0; i < entry->rowCount(); ++i)
			{
				writeEntry(writer, entry->getChild(i));
			}

			break;
		case FeedEntry:
			writer->writeAttribute(QLatin1String("xmlUrl"), entry->getRawData(UrlRole).toUrl().toString());
			writer->writeAttribute(QLatin1String("updateInterval"), QString::number(entry->getRawData(UpdateIntervalRole).toInt()));

			if (!entry->getRawData(Qt::DecorationRole).isNull())
			{
				writer->writeAttribute(QLatin1String("icon"), Utils::savePixmapAsDataUri(entry->icon().pixmap(entry->icon().availableSizes().value(0, {16, 16}))));
			}

			break;
		default:
			break;
	}

	writer->writeEndElement();
}

void FeedsModel::removeEntryUrl(Entry *entry)
{
	if (!entry)
	{
		return;
	}

	switch (entry->getType())
	{
		case FeedEntry:
			{
				const QUrl url(Utils::normalizeUrl(entry->data(UrlRole).toUrl()));

				if (!url.isEmpty() && m_urls.contains(url))
				{
					m_urls[url].removeAll(entry);

					if (m_urls[url].isEmpty())
					{
						m_urls.remove(url);
					}
				}
			}

			break;
		case FolderEntry:
			for (int i = 0; i < entry->rowCount(); ++i)
			{
				removeEntryUrl(entry->getChild(i));
			}

			break;
		default:
			break;
	}
}

void FeedsModel::readdEntryUrl(Entry *entry)
{
	if (!entry)
	{
		return;
	}

	switch (entry->getType())
	{
		case FeedEntry:
			{
				const QUrl url(Utils::normalizeUrl(entry->data(UrlRole).toUrl()));

				if (!url.isEmpty())
				{
					if (!m_urls.contains(url))
					{
						m_urls[url] = {};
					}

					m_urls[url].append(entry);
				}
			}

			break;
		case FolderEntry:
			for (int i = 0; i < entry->rowCount(); ++i)
			{
				readdEntryUrl(entry->getChild(i));
			}

			break;
		default:
			break;
	}
}

void FeedsModel::emptyTrash()
{
	if (m_trashEntry->rowCount() > 0)
	{
		m_trashEntry->removeRows(0, m_trashEntry->rowCount());
		m_trashEntry->setEnabled(false);

		m_trash.clear();

		emit modelModified();
	}
}

void FeedsModel::createIdentifier(FeedsModel::Entry *entry)
{
	const quint64 identifier(m_identifiers.isEmpty() ? 1 : (m_identifiers.lastKey() + 1));

	m_identifiers[identifier] = entry;

	entry->setData(identifier, IdentifierRole);
}

void FeedsModel::handleUrlChanged(Entry *entry, const QUrl &newUrl, const QUrl &oldUrl)
{
	if (!oldUrl.isEmpty() && m_urls.contains(oldUrl))
	{
		m_urls[oldUrl].removeAll(entry);

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

		m_urls[newUrl].append(entry);
	}
}

FeedsModel::Entry* FeedsModel::addEntry(EntryType type, const QMap<int, QVariant> &metaData, Entry *parent, int index)
{
	Entry *entry(new Entry());

	if (!parent)
	{
		parent = m_rootEntry;
	}

	parent->insertRow(((index < 0) ? parent->rowCount() : index), entry);

	if (type == FeedEntry)
	{
		entry->setDropEnabled(false);
	}

	if (type == FolderEntry || type == FeedEntry)
	{
		QMap<int, QVariant>::const_iterator iterator;

		for (iterator = metaData.begin(); iterator != metaData.end(); ++iterator)
		{
			setData(entry->index(), iterator.value(), iterator.key());
		}

		createIdentifier(entry);

		if (type == FeedEntry)
		{
			const QUrl url(metaData.value(UrlRole).toUrl());

			if (!url.isEmpty())
			{
				handleUrlChanged(entry, url);
			}

			entry->setFlags(entry->flags() | Qt::ItemNeverHasChildren);
		}
	}

	entry->setData(type, TypeRole);

	emit entryAdded(entry);
	emit modelModified();

	return entry;
}

FeedsModel::Entry* FeedsModel::addEntry(Feed *feed, Entry *parent, int index)
{
	if (!parent)
	{
		parent = m_rootEntry;
	}

	Entry *entry(new Entry(feed));
	entry->setData(FeedEntry, TypeRole);
	entry->setDropEnabled(false);

	createIdentifier(entry);

	parent->insertRow(((index < 0) ? parent->rowCount() : index), entry);

	const QUrl url(feed->getUrl());

	if (!url.isEmpty())
	{
		handleUrlChanged(entry, url);
	}

	entry->setFlags(entry->flags() | Qt::ItemNeverHasChildren);

	emit entryAdded(entry);
	emit modelModified();

	return entry;
}

FeedsModel::Entry* FeedsModel::getEntry(const QModelIndex &index) const
{
	Entry *entry(static_cast<Entry*>(itemFromIndex(index)));

	if (entry)
	{
		return entry;
	}

	return getEntry(index.data(IdentifierRole).toULongLong());
}

FeedsModel::Entry* FeedsModel::getEntry(quint64 identifier) const
{
	if (identifier == 0)
	{
		return m_rootEntry;
	}

	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	return nullptr;
}

FeedsModel::Entry* FeedsModel::getRootEntry() const
{
	return m_rootEntry;
}

FeedsModel::Entry* FeedsModel::getTrashEntry() const
{
	return m_trashEntry;
}

QMimeData* FeedsModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData(new QMimeData());
	QStringList texts;
	texts.reserve(indexes.count());

	QList<QUrl> urls;
	urls.reserve(indexes.count());

	if (indexes.count() == 1)
	{
		mimeData->setProperty("x-item-index", indexes.at(0));
	}

	for (const QModelIndex &index: indexes)
	{
		if (index.isValid() && static_cast<EntryType>(index.data(TypeRole).toInt()) == FeedEntry)
		{
			texts.append(index.data(UrlRole).toString());
			urls.append(index.data(UrlRole).toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	return mimeData;
}

QStringList FeedsModel::mimeTypes() const
{
	return {QLatin1String("text/uri-list")};
}

QVector<FeedsModel::Entry*> FeedsModel::getEntries(const QUrl &url) const
{
	const QUrl normalizedUrl(Utils::normalizeUrl(url));
	QVector<FeedsModel::Entry*> entries;

	if (m_urls.contains(url))
	{
		entries = m_urls[url];
	}

	if (url != normalizedUrl && m_urls.contains(normalizedUrl))
	{
		entries.append(m_urls[normalizedUrl]);
	}

	return entries;
}

bool FeedsModel::moveEntry(Entry *entry, Entry *newParent, int newRow)
{
	if (!entry || !newParent || entry == newParent || entry->isAncestorOf(newParent))
	{
		return false;
	}

	Entry *previousParent(static_cast<Entry*>(entry->parent()));

	if (!previousParent)
	{
		if (newRow < 0)
		{
			newParent->appendRow(entry);
		}
		else
		{
			newParent->insertRow(newRow, entry);
		}

		emit modelModified();

		return true;
	}

	const int previousRow(entry->row());

	if (newRow < 0)
	{
		newParent->appendRow(entry->parent()->takeRow(entry->row()));

		emit entryMoved(entry, previousParent, previousRow);
		emit modelModified();

		return true;
	}

	int targetRow(newRow);

	if (entry->parent() == newParent && entry->row() < newRow)
	{
		--targetRow;
	}

	newParent->insertRow(targetRow, entry->parent()->takeRow(entry->row()));

	emit entryMoved(entry, previousParent, previousRow);
	emit modelModified();

	return true;
}

bool FeedsModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (index.isValid())
	{
		const Entry *entry(getEntry(index));
		Entry *newParent(getEntry(parent));

		return (entry && newParent && entry != newParent && !entry->isAncestorOf(newParent));
	}

	return QStandardItemModel::canDropMimeData(data, action, row, column, parent);
}

bool FeedsModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (!Entry::isFolder(static_cast<EntryType>(parent.data(TypeRole).toInt())))
	{
		return false;
	}

	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (index.isValid())
	{
		return moveEntry(getEntry(index), getEntry(parent), row);
	}

	if (data->hasUrls())
	{
		const QVector<QUrl> urls(Utils::extractUrls(data));

		for (const QUrl &url: urls)
		{
			addEntry(FeedEntry, {{UrlRole, url}, {TitleRole, (data->property("x-url-title").toString().isEmpty() ? url.toString() : data->property("x-url-title").toString())}}, getEntry(parent), row);
		}

		return true;
	}

	return QStandardItemModel::dropMimeData(data, action, row, column, parent);
}

bool FeedsModel::save(const QString &path) const
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
	writer.writeStartElement(QLatin1String("opml"));
	writer.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));
	writer.writeStartElement(QLatin1String("head"));
	writer.writeTextElement(QLatin1String("title"), QLatin1String("Newsfeeds exported from Otter Browser ") + Application::getFullVersion());
	writer.writeEndElement();
	writer.writeStartElement(QLatin1String("body"));

	for (int i = 0; i < m_rootEntry->rowCount(); ++i)
	{
		writeEntry(&writer, m_rootEntry->getChild(i));
	}

	writer.writeEndElement();
	writer.writeEndDocument();

	return file.commit();
}

bool FeedsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Entry *entry(getEntry(index));

	if (!entry)
	{
		return QStandardItemModel::setData(index, value, role);
	}

	if (role == UrlRole)
	{
		const QUrl oldUrl(index.data(UrlRole).toUrl());
		const QUrl newUrl(value.toUrl());

		if (oldUrl != newUrl)
		{
			handleUrlChanged(entry, Utils::normalizeUrl(newUrl), Utils::normalizeUrl(oldUrl));
		}
	}

	entry->setData(value, role);

	switch (role)
	{
		case TitleRole:
		case UrlRole:
		case DescriptionRole:
		case IdentifierRole:
		case TypeRole:
			emit entryModified(entry);
			emit modelModified();

			break;
		default:
			break;
	}

	return true;
}

bool FeedsModel::hasFeed(const QUrl &url) const
{
	return (m_urls.contains(url) || m_urls.contains(Utils::normalizeUrl(url)));
}

}
