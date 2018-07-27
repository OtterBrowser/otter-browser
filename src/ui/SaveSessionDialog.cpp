/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "SaveSessionDialog.h"
#include "MainWindow.h"
#include "../core/SessionsManager.h"
#include "../core/Utils.h"

#include "ui_SaveSessionDialog.h"

#include <QtGui/QRegularExpressionValidator>
#include <QtWidgets/QMessageBox>

namespace Otter
{

SaveSessionDialog::SaveSessionDialog(QWidget *parent) : Dialog(parent),
	m_ui(new Ui::SaveSessionDialog)
{
	QString identifier(SessionsManager::getCurrentSession());

	if (identifier == QLatin1String("default"))
	{
		identifier = Utils::createIdentifier(QLatin1String("default"), SessionsManager::getSessions());
	}

	m_ui->setupUi(this);
	m_ui->titleLineEditWidget->setText(SessionsManager::getSession(SessionsManager::getCurrentSession()).title);
	m_ui->identifierLineEditWidget->setText(identifier);
	m_ui->identifierLineEditWidget->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("[a-z0-9\\-_]+")), this));

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &SaveSessionDialog::saveSession);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &SaveSessionDialog::close);
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
	}
}

void SaveSessionDialog::saveSession()
{
	const QString identifier(m_ui->identifierLineEditWidget->text());

	if (identifier.isEmpty() || (SessionsManager::getCurrentSession() != identifier && SessionsManager::getSession(identifier).isValid() && QMessageBox::question(this, tr("Question"), tr("Session with specified indentifier already exists.\nDo you want to overwrite it?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No))
	{
		show();

		return;
	}

	if (SessionsManager::saveSession(identifier, m_ui->titleLineEditWidget->text(), (m_ui->onlyCurrentWindowCheckBox->isChecked() ? qobject_cast<MainWindow*>(parentWidget()) : nullptr)))
	{
		close();
	}
	else
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to save session."), QMessageBox::Close);
	}
}

}
