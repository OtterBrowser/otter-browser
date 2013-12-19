#ifndef OTTER_SEARCHPROPERTIESDIALOG_H
#define OTTER_SEARCHPROPERTIESDIALOG_H

#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class SearchPropertiesDialog;
}

class SearchPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SearchPropertiesDialog(QWidget *parent = NULL);
	~SearchPropertiesDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::SearchPropertiesDialog *m_ui;
};

}

#endif
