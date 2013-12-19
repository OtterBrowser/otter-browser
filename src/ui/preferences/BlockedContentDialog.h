#ifndef OTTER_BLOCKEDCONTENTDIALOG_H
#define OTTER_BLOCKEDCONTENTDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class BlockedContentDialog;
}

class BlockedContentDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BlockedContentDialog(QWidget *parent = NULL);
	~BlockedContentDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::BlockedContentDialog *m_ui;
};

}

#endif

