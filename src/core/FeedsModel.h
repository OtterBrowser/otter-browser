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

#ifndef OTTER_FEEDSMODEL_H
#define OTTER_FEEDSMODEL_H

#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class Feed;

class FeedsModel final : public QStandardItemModel
{
	Q_OBJECT

public:
	enum EntryType
	{
		UnknownEntry = 0,
		RootEntry,
		TrashEntry,
		FolderEntry,
		FeedEntry,
		CategoryEntry
	};

	enum EntryRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		DescriptionRole = Qt::ToolTipRole,
		IdentifierRole = Qt::UserRole,
		TypeRole,
		LastUpdateTimeRole,
		LastSynchronizationTimeRole,
		UnreadEntriesAmountRole,
		UpdateIntervalRole,
		UpdateProgressValueRole,
		IsShowingProgressIndicatorRole,
		IsTrashedRole,
		IsUpdatingRole,
		HasErrorRole,
		UserRole
	};

	class Entry final : public QStandardItem
	{
	public:
		Entry* getChild(int index) const;
		Feed* getFeed() const;
		QString getTitle() const;
		QVariant data(int role) const override;
		QVariant getRawData(int role) const;
		QVector<Feed*> getFeeds() const;
		quint64 getIdentifier() const;
		EntryType getType() const;
		static bool isFolder(EntryType type);
		bool isFolder() const;
		bool isAncestorOf(Entry *child) const;
		bool operator<(const QStandardItem &other) const override;

	protected:
		explicit Entry(Feed *feed = nullptr);

	private:
		Feed *m_feed;

	friend class FeedsModel;
	};

	explicit FeedsModel(const QString &path, QObject *parent = nullptr);

	void beginImport(Entry *target, int estimatedUrlsAmount);
	void endImport();
	void trashEntry(Entry *entry);
	void restoreEntry(Entry *entry);
	void removeEntry(Entry *entry);
	Entry* addEntry(EntryType type, const QMap<int, QVariant> &metaData = {}, Entry *parent = nullptr, int index = -1);
	Entry* addEntry(Feed *feed, Entry *parent = nullptr, int index = -1);
	Entry* getEntry(const QModelIndex &index) const;
	Entry* getEntry(quint64 identifier) const;
	Entry* getRootEntry() const;
	Entry* getTrashEntry() const;
	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;
	QVector<Entry*> getEntries(const QUrl &url) const;
	bool moveEntry(Entry *entry, Entry *newParent, int newRow = -1);
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	bool hasFeed(const QUrl &url) const;

public slots:
	void emptyTrash();

protected:
	struct EntryLocation final
	{
		QModelIndex parent;
		int row = -1;
	};

	void readEntry(QXmlStreamReader *reader, Entry *parent);
	void writeEntry(QXmlStreamWriter *writer, Entry *entry) const;
	void removeEntryUrl(Entry *entry);
	void readdEntryUrl(Entry *entry);
	void createIdentifier(Entry *entry);
	void handleUrlChanged(Entry *entry, const QUrl &newUrl, const QUrl &oldUrl = {});

private:
	Entry *m_rootEntry;
	Entry *m_trashEntry;
	Entry *m_importTargetEntry;
	QHash<Entry*, EntryLocation> m_trash;
	QHash<QUrl, QVector<Entry*> > m_urls;
	QMap<quint64, Entry*> m_identifiers;

signals:
	void entryAdded(Entry *entry);
	void entryModified(Entry *entry);
	void entryMoved(Entry *entry, Entry *previousParent, int previousRow);
	void entryTrashed(Entry *entry, Entry *previousParent);
	void entryRestored(Entry *entry);
	void entryRemoved(Entry *entry, Entry *previousParent);
	void modelModified();
};

}

#endif
