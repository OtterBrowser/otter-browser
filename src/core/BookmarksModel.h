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

#ifndef OTTER_BOOKMARKSMODEL_H
#define OTTER_BOOKMARKSMODEL_H

#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class BookmarksModel;

class BookmarksItem : public QStandardItem
{
public:
	enum BookmarkType
	{
		UnknownBookmark = 0,
		RootBookmark = 1,
		TrashBookmark = 2,
		FolderBookmark = 3,
		UrlBookmark = 4,
		SeparatorBookmark = 5
	};

	~BookmarksItem();

	BookmarksModel* getModel() const;
	QStandardItem* clone() const;
	QVariant data(int role) const;

protected:
	explicit BookmarksItem(BookmarkType type, const QUrl &url = QUrl(), const QString &title = QString());

	friend class BookmarksModel;
};

class BookmarksModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum BookmarkRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		DescriptionRole = Qt::ToolTipRole,
		IdentifierRole = Qt::UserRole,
		TypeRole = (Qt::UserRole + 1),
		KeywordRole = (Qt::UserRole + 2),
		TimeAddedRole = (Qt::UserRole + 3),
		TimeModifiedRole = (Qt::UserRole + 4),
		TimeVisitedRole = (Qt::UserRole + 5),
		VisitsRole = (Qt::UserRole + 6)
	};

	explicit BookmarksModel(const QString &path, QObject *parent = NULL);

	BookmarksItem* addBookmark(BookmarksItem::BookmarkType type, quint64 identifier = 0, const QUrl &url = QUrl(), const QString &title = QString(), BookmarksItem *parent = NULL);
	BookmarksItem* bookmarkFromIndex(const QModelIndex &index) const;
	BookmarksItem* getBookmark(const QString &keyword) const;
	BookmarksItem* getBookmark(quint64 identifier) const;
	BookmarksItem* getRootItem() const;
	BookmarksItem* getTrashItem() const;
	BookmarksItem* getItem(const QString &path) const;
	QMimeData* mimeData(const QModelIndexList &indexes) const;
	QStringList mimeTypes() const;
	QStringList getKeywords() const;
	QList<BookmarksItem*> getBookmarks(const QUrl &url) const;
	QList<BookmarksItem*> findUrls(const QUrl &url, QStandardItem *branch = NULL) const;
	QList<QUrl> getUrls() const;
	static QUrl adjustUrl(QUrl url);
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	bool hasBookmark(const QUrl &url) const;
	bool hasKeyword(const QString &keyword) const;
	bool hasUrl(const QUrl &url) const;

protected:
	void removeBookmark(BookmarksItem *bookmark);
	void readBookmark(QXmlStreamReader *reader, BookmarksItem *parent);
	void writeBookmark(QXmlStreamWriter *writer, QStandardItem *bookmark) const;

private:
	QHash<QUrl, QList<BookmarksItem*> > m_urls;
	QHash<QString, BookmarksItem*> m_keywords;
	QMap<quint64, BookmarksItem*> m_identifiers;

friend class BookmarksItem;
};

}

#endif
