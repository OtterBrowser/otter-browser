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

#ifndef OTTER_BOOKMARKSCONTENTSWIDGET_H
#define OTTER_BOOKMARKSCONTENTSWIDGET_H

#include "../../../core/BookmarksManager.h"
#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class BookmarksContentsWidget;
}

class Window;

class BookmarksContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit BookmarksContentsWidget(Window *window);
	~BookmarksContentsWidget();

	void print(QPrinter *printer);
	Action* getAction(int identifier);
	QString getTitle() const;
	QLatin1String getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void triggerAction(int identifier, bool checked = false);

protected:
	void changeEvent(QEvent *event);
	QStandardItem* findFolder(const QModelIndex &index);
	bool isInTrash(const QModelIndex &index) const;

protected slots:
	void triggerAction();
	void addBookmark();
	void addFolder();
	void addSeparator();
	void removeBookmark();
	void restoreBookmark();
	void openBookmark(const QModelIndex &index = QModelIndex());
	void copyBookmarkLink();
	void bookmarkProperties();
	void emptyTrash();
	void showContextMenu(const QPoint &point);
	void updateActions();
	bool filterBookmarks(const QString &filter, QStandardItem *branch = NULL);

private:
	QHash<int, Action*> m_actions;
	QHash<QStandardItem*, QPair<QModelIndex, int> > m_trash;
	Ui::BookmarksContentsWidget *m_ui;
};

}

#endif
