#include "SearchPropertiesDialog.h"

#include "ui_SearchPropertiesDialog.h"

namespace Otter
{

SearchPropertiesDialog::SearchPropertiesDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::SearchPropertiesDialog)
{
	m_ui->setupUi(this);
}

SearchPropertiesDialog::~SearchPropertiesDialog()
{
	delete m_ui;
}

void SearchPropertiesDialog::changeEvent(QEvent *event)
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
