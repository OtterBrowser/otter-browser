/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "JavaScriptPreferencesDialog.h"

#include "ui_JavaScriptPreferencesDialog.h"

namespace Otter
{

JavaScriptPreferencesDialog::JavaScriptPreferencesDialog(const QVariantMap &options, QWidget *parent) : QDialog(parent),
	m_ui(new Ui::JavaScriptPreferencesDialog)
{
	m_ui->setupUi(this);
	m_ui->javaScriptCanShowStatusMessagesCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanShowStatusMessages")).toBool());
	m_ui->javaScriptCanAccessClipboardCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanAccessClipboard")).toBool());
	m_ui->javaScriptCanDisableContextMenuCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanDisableContextMenu")).toBool());

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

JavaScriptPreferencesDialog::~JavaScriptPreferencesDialog()
{
	delete m_ui;
}

void JavaScriptPreferencesDialog::changeEvent(QEvent *event)
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

QVariantMap JavaScriptPreferencesDialog::getOptions() const
{
	QVariantMap options;
	options[QLatin1String("Browser/JavaScriptCanShowStatusMessages")] = m_ui->javaScriptCanShowStatusMessagesCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanAccessClipboard")] = m_ui->javaScriptCanAccessClipboardCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanDisableContextMenu")] = m_ui->javaScriptCanDisableContextMenuCheckBox->isChecked();

	return options;
}

}
