#ifndef OTTER_SAVESESSIONDIALOG_H
#define OTTER_SAVESESSIONDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class SaveSessionDialog;
}

class SaveSessionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SaveSessionDialog(QWidget *parent = NULL);
	~SaveSessionDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void saveSession();

private:
	Ui::SaveSessionDialog *m_ui;
};

}

#endif
