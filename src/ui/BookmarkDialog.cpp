#include "BookmarkDialog.h"

#include "ui_BookmarkDialog.h"

namespace Otter
{

BookmarkDialog::BookmarkDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::BookmarkDialog)
{
	m_ui->setupUi(this);
}

BookmarkDialog::~BookmarkDialog()
{
	delete m_ui;
}

void BookmarkDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

}
