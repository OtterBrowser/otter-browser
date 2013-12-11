#ifndef OTTER_BOOKMARKPROPERTIESDIALOG_H
#define OTTER_BOOKMARKPROPERTIESDIALOG_H

#include "../core/BookmarksManager.h"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class BookmarkPropertiesDialog;
}

class BookmarkPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BookmarkPropertiesDialog(BookmarkInformation *bookmark, int folder, QWidget *parent = NULL);
	~BookmarkPropertiesDialog();

protected:
	void changeEvent(QEvent *event);
	void populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent);

protected slots:
	void folderChanged(const QModelIndex &index);
	void createFolder();
	void reloadFolders();
	void saveBookmark();

private:
	BookmarkInformation *m_bookmark;
	QStandardItemModel *m_model;
	QModelIndex m_index;
	int m_folder;
	Ui::BookmarkPropertiesDialog *m_ui;
};

}

#endif
