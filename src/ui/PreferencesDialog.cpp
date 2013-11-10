#include "PreferencesDialog.h"

#include "ui_PreferencesDialog.h"

namespace Otter
{

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::PreferencesDialog)
{
	m_ui->setupUi(this);
}

PreferencesDialog::~PreferencesDialog()
{
	delete m_ui;
}

void PreferencesDialog::changeEvent(QEvent *event)
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
