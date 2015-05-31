/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OpenAddressDialog.h"
#include "toolbars/AddressWidget.h"
#include "../core/InputInterpreter.h"

#include "ui_OpenAddressDialog.h"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QPushButton>

namespace Otter
{

OpenAddressDialog::OpenAddressDialog(QWidget *parent) : QDialog(parent),
	m_addressWidget(NULL),
	m_ui(new Ui::OpenAddressDialog)
{
	m_ui->setupUi(this);

	m_addressWidget = new AddressWidget(NULL, this);
	m_addressWidget->setFocus();

	m_ui->verticalLayout->insertWidget(1, m_addressWidget);
	m_ui->label->setBuddy(m_addressWidget);

	connect(this, SIGNAL(accepted()), this, SLOT(handleUserInput()));
}

OpenAddressDialog::~OpenAddressDialog()
{
	delete m_ui;
}

void OpenAddressDialog::changeEvent(QEvent *event)
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

void OpenAddressDialog::handleUserInput()
{
	if (m_addressWidget->text().trimmed().isEmpty())
	{
		close();
	}
	else
	{
		InputInterpreter *interpreter = new InputInterpreter(this);

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SIGNAL(requestedLoadUrl(QUrl,OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SIGNAL(requestedSearch(QString,QString,OpenHints)));
		connect(interpreter, SIGNAL(destroyed()), this, SLOT(accept()));

		interpreter->interpret(m_addressWidget->text(), WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers(), Qt::LeftButton, CurrentTabOpen));
	}
}

void OpenAddressDialog::setText(const QString &text)
{
	m_addressWidget->setText(text);
	m_addressWidget->clearFocus();
	m_addressWidget->setFocus(Qt::OtherFocusReason);
}

}
