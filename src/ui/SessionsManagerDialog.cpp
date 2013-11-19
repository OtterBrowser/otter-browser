#include "SessionsManagerDialog.h"

#include "ui_SessionsManagerDialog.h"

namespace Otter
{

SessionsManagerDialog::SessionsManagerDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::SessionsManagerDialog)
{
	m_ui->setupUi(this);
}

SessionsManagerDialog::~SessionsManagerDialog()
{
	delete m_ui;
}

void SessionsManagerDialog::changeEvent(QEvent *event)
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
