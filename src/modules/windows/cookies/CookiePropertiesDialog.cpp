#include "CookiePropertiesDialog.h"

#include "ui_CookiePropertiesDialog.h"

namespace Otter
{

CookiePropertiesDialog::CookiePropertiesDialog(const QNetworkCookie &cookie, QWidget *parent) : QDialog(parent),
	m_ui(new Ui::CookiePropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->domainLineEdit->setText(cookie.domain());
	m_ui->nameLineEdit->setText(QString(cookie.name()));
	m_ui->valueLineEdit->setText(QString(cookie.value()));
	m_ui->expiresDateTimeEdit->setDateTime(cookie.expirationDate());
	m_ui->secureCheckBox->setChecked(cookie.isSecure());
	m_ui->httpOnlyCheckBox->setChecked(cookie.isHttpOnly());
}

CookiePropertiesDialog::~CookiePropertiesDialog()
{
	delete m_ui;
}

void CookiePropertiesDialog::changeEvent(QEvent *event)
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
