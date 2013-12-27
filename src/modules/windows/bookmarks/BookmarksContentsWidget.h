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
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;

protected:
	void changeEvent(QEvent *event);
	void gatherBookmarks(int folder);
	QStandardItem* findFolder(int folder, QStandardItem *item = NULL);
	int findFolder(const QModelIndex &index);

protected slots:
	void addBookmark(BookmarkInformation *bookmark, QStandardItem *parent = NULL);
	void addBookmark();
	void addFolder();
	void addSeparator();
	void removeBookmark();
	void openBookmark(const QModelIndex &index = QModelIndex());
	void copyBookmarkLink();
	void bookmarkProperties();
	void showContextMenu(const QPoint &point);
	void updateFolder(int folder);
	void updateActions();
	bool filterBookmarks(const QString &filter, QStandardItem *branch = NULL);

private:
	QStandardItemModel *m_model;
	QList<QString> m_bookmarksToOpen;
	QHash<WindowAction, QAction*> m_actions;
	Ui::BookmarksContentsWidget *m_ui;
};

}

#endif
