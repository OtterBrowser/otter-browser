#ifndef OTTER_BOOKMARKDIALOG_H
#define OTTER_BOOKMARKDIALOG_H

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
	explicit BookmarkDialog(QWidget *parent = NULL);
	~BookmarkDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::BookmarkDialog *m_ui;
};

}

#endif
