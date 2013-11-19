#ifndef OTTER_SESSIONSMANAGERDIALOG_H
#define OTTER_SESSIONSMANAGERDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class SessionsManagerDialog;
}

class SessionsManagerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SessionsManagerDialog(QWidget *parent = NULL);
	~SessionsManagerDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::SessionsManagerDialog *m_ui;
};

}

#endif
