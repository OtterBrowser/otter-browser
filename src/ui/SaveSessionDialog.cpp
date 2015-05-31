/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

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
	m_ui->identifierLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("[a-z0-9\\-_]+")), this));

	adjustSize();

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

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		adjustSize();
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
