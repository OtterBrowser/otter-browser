#ifndef OTTER_COOKIESDIALOG_H
#define OTTER_COOKIESDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
class CookiesDialog;
}

class CookiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CookiesDialog(QWidget *parent = NULL);
	~CookiesDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::CookiesDialog *m_ui;
};

}

#endif
