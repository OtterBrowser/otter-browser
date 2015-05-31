/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_STARTPAGEMODEL_H
#define OTTER_STARTPAGEMODEL_H

#include <QtGui/QStandardItemModel>

namespace Otter
{

class BookmarksItem;

class StartPageModel : public QStandardItemModel
{
	Q_OBJECT

public:
	explicit StartPageModel(QObject *parent = NULL);

	QMimeData* mimeData(const QModelIndexList &indexes) const;
	QStringList mimeTypes() const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
	bool isReloadingTile(const QModelIndex &index) const;

public slots:
	void reloadModel();
	void reloadTile(const QModelIndex &index, bool full = false);

protected slots:
	void optionChanged(const QString &option);
	void dragEnded();
	void thumbnailCreated(const QUrl &url, const QPixmap &thumbnail, const QString &title);

private:
	BookmarksItem *m_bookmark;
	QHash<QUrl, QPair<quint64, bool> > m_reloads;

signals:
	void modelModified();
	void isReloadingTileChanged(const QModelIndex &index);
};

}

#endif
