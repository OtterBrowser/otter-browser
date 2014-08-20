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

#ifndef OTTER_BOOKMARKSMANAGER_H
#define OTTER_BOOKMARKSMANAGER_H

#include <QtCore/QDateTime>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QStandardItemModel>

namespace Otter
{

enum BookmarkType
{
	UnknownBookmark = 0,
	RootBookmark = 1,
	TrashBookmark = 2,
	FolderBookmark = 3,
	UrlBookmark = 4,
	SeparatorBookmark = 5
};

class BookmarksItem;
class BookmarksModel;

class BookmarksManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void updateVisits(const QString &url);
	static void deleteBookmark(const QString &url);
	static BookmarksManager* getInstance();
	static BookmarksModel* getModel();
	static BookmarksItem* getBookmark(const QString &keyword);
	static QStringList getKeywords();
	static QStringList getUrls();
	static bool hasBookmark(const QString &url);
	static bool hasKeyword(const QString &keyword);
	static bool save(const QString &path = QString());

protected:
	explicit BookmarksManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void readBookmark(QXmlStreamReader *reader, BookmarksItem *parent);
	static void writeBookmark(QXmlStreamWriter *writer, QStandardItem *bookmark);

protected slots:
	void scheduleSave();
	void load();

private:
	int m_saveTimer;

	static BookmarksManager *m_instance;
	static BookmarksModel* m_model;

signals:
	void modelModified();
};

}

#endif
