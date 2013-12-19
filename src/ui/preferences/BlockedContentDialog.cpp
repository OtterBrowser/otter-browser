#include "BlockedContentDialog.h"

#include "ui_BlockedContentDialog.h"

namespace Otter
{

BlockedContentDialog::BlockedContentDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::BlockedContentDialog)
{
	m_ui->setupUi(this);
}

BlockedContentDialog::~BlockedContentDialog()
{
	delete m_ui;
}

void BlockedContentDialog::changeEvent(QEvent *event)
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
