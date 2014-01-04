#ifndef OTTER_BOOKMARKSIMPORTDIALOG_H
#define OTTER_BOOKMARKSIMPORTDIALOG_H

#include "../core/BookmarksManager.h"

#include <QtWidgets/QDialog>
#include <QtWidgets/QInputDialog>
#include <QtGui/QStandardItemModel>
#include <QTreeView>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QStack>

namespace Otter {

namespace Ui {
	class BookmarksImportDialog;
}

enum EntryType
{
	EntryBookmark = 0,
	EntryFolder = 1,
	EntryFolderEnd = 2,
	EntrySeparator = 3
};


class BookmarksImportDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BookmarksImportDialog(QWidget *parent = 0);
	~BookmarksImportDialog();

protected:
	void populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent);

protected slots:
	void ready();
	void chooseFile();
	void folderChanged(const QModelIndex &index);
	void createFolder();
	void reloadFolders();

private:
	QStandardItemModel *m_model;
	QModelIndex m_index;
	BookmarkInformation *bookmark;
	QStack<int> foldersPath;
	int m_folder;
	int currentEntryType;
	QString m_filename;
	Ui::BookmarksImportDialog *m_ui;
	int importBookmarksFromAdr(QTextStream &istream);
	void atReadingStart();
	void atFolderStart(BookmarkInformation bookmark);
	void atFolderEnd();
	void atSeparator(BookmarkInformation bookmark);
	void atBookmark(BookmarkInformation bookmark);
	void atUnknownLine(QString line);
};


}
#endif
