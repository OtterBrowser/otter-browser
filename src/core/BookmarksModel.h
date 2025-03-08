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

#ifndef OTTER_BOOKMARKSMODEL_H
#define OTTER_BOOKMARKSMODEL_H

#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class Feed;

class BookmarksModel final : public QStandardItemModel
{
	Q_OBJECT

public:
	enum BookmarkType
	{
		UnknownBookmark = 0,
		RootBookmark,
		TrashBookmark,
		FeedBookmark,
		FolderBookmark,
		UrlBookmark,
		SeparatorBookmark
	};

	enum BookmarkRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		DescriptionRole = Qt::ToolTipRole,
		IdentifierRole = Qt::UserRole,
		TypeRole,
		KeywordRole,
		TimeAddedRole,
		TimeModifiedRole,
		TimeVisitedRole,
		VisitsRole,
		IsTrashedRole,
		UserRole
	};

	enum FormatMode
	{
		BookmarksMode = 0,
		NotesMode
	};

	class Bookmark final : public QStandardItem
	{
	public:
		void remove();
		void setData(const QVariant &value, int role) override;
		void setItemData(const QVariant &value, int role);
		QStandardItem* clone() const override;
		Bookmark* getParent() const;
		Bookmark* getChild(int index) const;
		QString getTitle() const;
		QString getDescription() const;
		QString getKeyword() const;
		QUrl getUrl() const;
		QDateTime getTimeAdded() const;
		QDateTime getTimeModified() const;
		QDateTime getTimeVisited() const;
		QIcon getIcon() const;
		QVariant data(int role) const override;
		QVariant getRawData(int role) const;
		QVector<QUrl> getUrls() const;
		quint64 getIdentifier() const;
		BookmarkType getType() const;
		int getVisits() const;
		bool hasChildren() const;
		static bool isFolder(BookmarkType type);
		bool isFolder() const;
		bool isAncestorOf(Bookmark *child) const;
		bool operator<(const QStandardItem &other) const override;

	protected:
		explicit Bookmark();

	friend class BookmarksModel;
	};

	struct BookmarkMatch final
	{
		Bookmark *bookmark = nullptr;
		QString match;
	};

	explicit BookmarksModel(const QString &path, FormatMode mode, QObject *parent = nullptr);

	void beginImport(Bookmark *target, int estimatedUrlsAmount = 0, int estimatedKeywordsAmount = 0);
	void endImport();
	void trashBookmark(Bookmark *bookmark);
	void restoreBookmark(Bookmark *bookmark);
	void removeBookmark(Bookmark *bookmark);
	Bookmark* addBookmark(BookmarkType type, const QMap<int, QVariant> &metaData = {}, Bookmark *parent = nullptr, int index = -1);
	Bookmark* getBookmarkByKeyword(const QString &keyword) const;
	Bookmark* getBookmarkByPath(const QString &path, bool createIfNotExists = false);
	Bookmark* getBookmark(const QModelIndex &index) const;
	Bookmark* getBookmark(quint64 identifier) const;
	Bookmark* getRootItem() const;
	Bookmark* getTrashItem() const;
	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;
	QStringList getKeywords() const;
	QVector<BookmarkMatch> findBookmarks(const QString &prefix) const;
	QVector<Bookmark*> findUrls(const QUrl &url, Bookmark *branch = nullptr) const;
	QVector<Bookmark*> getBookmarks(const QUrl &url) const;
	FormatMode getFormatMode() const;
	int getCount() const;
	bool moveBookmark(Bookmark *bookmark, Bookmark *newParent, int newRow = -1);
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	bool hasBookmark(const QUrl &url) const;
	bool hasFeed(const QUrl &url) const;
	bool hasKeyword(const QString &keyword) const;

public slots:
	void emptyTrash();

protected:
	struct BookmarkLocation final
	{
		QModelIndex parent;
		int row = -1;
	};

	void readBookmark(QXmlStreamReader *reader, Bookmark *parent);
	void writeBookmark(QXmlStreamWriter *writer, Bookmark *bookmark) const;
	void removeBookmarkUrl(Bookmark *bookmark);
	void readdBookmarkUrl(Bookmark *bookmark);
	void setupFeed(Bookmark *bookmark);
	void handleKeywordChanged(Bookmark *bookmark, const QString &newKeyword, const QString &oldKeyword = {});
	void handleUrlChanged(Bookmark *bookmark, const QUrl &newUrl, const QUrl &oldUrl = {});
	static QDateTime readDateTime(QXmlStreamReader *reader, const QString &attribute);

protected slots:
	void handleFeedModified(Feed *feed);
	void notifyBookmarkModified(const QModelIndex &index);

private:
	Bookmark *m_rootItem;
	Bookmark *m_trashItem;
	Bookmark *m_importTargetItem;
	QHash<Bookmark*, BookmarkLocation> m_trash;
	QHash<QUrl, QVector<Bookmark*> > m_feeds;
	QHash<QUrl, QVector<Bookmark*> > m_urls;
	QHash<QString, Bookmark*> m_keywords;
	QMap<quint64, Bookmark*> m_identifiers;
	FormatMode m_mode;

signals:
	void bookmarkAdded(Bookmark *bookmark);
	void bookmarkModified(Bookmark *bookmark);
	void bookmarkMoved(Bookmark *bookmark, Bookmark *previousParent, int previousRow);
	void bookmarkTrashed(Bookmark *bookmark, Bookmark *previousParent);
	void bookmarkRestored(Bookmark *bookmark);
	void bookmarkRemoved(Bookmark *bookmark, Bookmark *previousParent);
	void modelModified();

friend class Bookmark;
};

}

#endif
