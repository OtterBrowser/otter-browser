#include "SaveSessionDialog.h"
#include "MainWindow.h"
#include "../core/SessionsManager.h"

#include "ui_SaveSessionDialog.h"

#include <QtGui/QRegularExpressionValidator>
#include <QtWidgets/QMessageBox>

namespace Otter
{

SaveSessionDialog::SaveSessionDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::SaveSessionDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEdit->setText(SessionsManager::getSession(SessionsManager::getCurrentSession()).title);
	m_ui->identifierLineEdit->setText(SessionsManager::getCurrentSession());
	m_ui->identifierLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-z0-9\\-_]+"), this));

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(saveSession()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

SaveSessionDialog::~SaveSessionDialog()
{
	delete m_ui;
}

void SaveSessionDialog::changeEvent(QEvent *event)
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

void SaveSessionDialog::saveSession()
{
	if (m_ui->identifierLineEdit->text().isEmpty())
	{
		show();

		return;
	}

	if (m_ui->identifierLineEdit->text() != SessionsManager::getCurrentSession() && SessionsManager::getSession(m_ui->identifierLineEdit->text()).windows.count() > 0 && QMessageBox::question(this, tr("Question"), tr("Session with specified indentifier already exists.\nDo you want to overwrite it?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		show();

		return;
	}

	if (SessionsManager::saveSession(m_ui->identifierLineEdit->text(), m_ui->titleLineEdit->text(), (m_ui->onlyCurrentWindowCheckBox->isChecked() ? qobject_cast<MainWindow*>(parentWidget()) : NULL)))
	{
		close();
	}
	else
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to save session."), QMessageBox::Close);
	}
}

}
