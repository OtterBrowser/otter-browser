#ifndef OTTER_BOOKMARKDIALOG_H
#define OTTER_BOOKMARKDIALOG_H

#include "../core/BookmarksManager.h"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class BookmarkDialog;
}

class BookmarkDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BookmarkDialog(Bookmark *bookmark, QWidget *parent = NULL);
	~BookmarkDialog();

protected:
	void changeEvent(QEvent *event);
	void populateFolder(const QList<Bookmark*> bookmarks, QStandardItem *parent);

protected slots:
	void folderChanged(const QModelIndex &index);
	void createFolder();
	void reloadFolders();
	void saveBookmark();

private:
	Bookmark *m_bookmark;
	QStandardItemModel *m_model;
	QModelIndex m_index;
	int m_folder;
	Ui::BookmarkDialog *m_ui;
};

}

#endif
