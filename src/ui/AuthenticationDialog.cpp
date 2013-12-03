#include "AuthenticationDialog.h"

#include "ui_AuthenticationDialog.h"

namespace Otter
{

AuthenticationDialog::AuthenticationDialog(const QUrl &url, QAuthenticator *authenticator, QWidget *parent) : QDialog(parent),
	m_authenticator(authenticator),
	m_ui(new Ui::AuthenticationDialog)
{
	m_ui->setupUi(this);
	m_ui->serverValueLabel->setText(url.host());
	m_ui->messageValueLabel->setText(authenticator->realm().toHtmlEscaped());
	m_ui->userLineEdit->setText(authenticator->user());
	m_ui->passwordLineEdit->setText(authenticator->password());

	connect(this, SIGNAL(accepted()), this, SLOT(setup()));
}

AuthenticationDialog::~AuthenticationDialog()
{
	delete m_ui;
}

void AuthenticationDialog::changeEvent(QEvent *event)
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

void AuthenticationDialog::setup()
{
	m_authenticator->setUser(m_ui->userLineEdit->text());
	m_authenticator->setPassword(m_ui->passwordLineEdit->text());
}

}
