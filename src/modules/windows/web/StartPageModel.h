/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_STARTPAGEMODEL_H
#define OTTER_STARTPAGEMODEL_H

#include "../../../core/BookmarksModel.h"

namespace Otter
{

class StartPageModel final : public QStandardItemModel
{
	Q_OBJECT

public:
	enum
	{
		IsDraggedRole = BookmarksModel::UserRole,
		IsEmptyRole,
		IsReloadingRole
	};

	explicit StartPageModel(QObject *parent = nullptr);

	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	static BookmarksModel::Bookmark* getBookmark(const QModelIndex &index);
	static QString getThumbnailPath(quint64 identifier);
	QVariant data(const QModelIndex &index, int role) const override;
	QStringList mimeTypes() const override;
	bool reloadTile(const QModelIndex &index, bool needsTitleUpdate = false);
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	bool event(QEvent *event) override;

public slots:
	void reloadModel();
	QModelIndex addTile(const QUrl &url);

protected:
	BookmarksModel::Bookmark* getRootBookmark() const;
	bool requestThumbnail(const QUrl &url, quint64 identifier, bool needsTitleUpdate = false);

protected slots:
	void handleOptionChanged(int identifier);
	void handleDragEnded();
	void handleBookmarkModified(BookmarksModel::Bookmark *bookmark);
	void handleBookmarkMoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent);
	void handleBookmarkRemoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent);
	void handleThumbnailCreated(quint64 identifier, const QPixmap &thumbnail, const QString &title);

private:
	BookmarksModel::Bookmark *m_bookmark;
	QHash<quint64, bool> m_tileReloads;

signals:
	void modelModified();
	void isReloadingTileChanged(const QModelIndex &index);
};

}

#endif
