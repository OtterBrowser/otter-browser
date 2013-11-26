#ifndef OTTER_BOOKMARKDIALOG_H
#define OTTER_BOOKMARKDIALOG_H

#include "../core/BookmarksManager.h"

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

protected slots:
	void createFolder();

private:
	Bookmark *m_bookmark;
	Ui::BookmarkDialog *m_ui;
};

}

#endif
