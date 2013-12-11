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
	ContentsWidget* clone(Window *window = NULL);
	QAction* getAction(WindowAction action);
	QUndoStack* getUndoStack();
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	HistoryInformation getHistory() const;
	int getZoom() const;
	bool canZoom() const;
	bool isClonable() const;
	bool isLoading() const;
	bool isPrivate() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);
	void setHistory(const HistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url);

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
	void deleteBookmark();
	void openBookmark(const QModelIndex &index);
	void bookmarkProperties();
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
