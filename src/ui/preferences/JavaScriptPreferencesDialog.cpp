/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	m_ui->canChangeWindowGeometryCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry")).toBool());
	m_ui->canShowStatusMessagesCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanShowStatusMessages")).toBool());
	m_ui->canAccessClipboardCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanAccessClipboard")).toBool());
	m_ui->canDisableContextMenuCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanDisableContextMenu")).toBool());
	m_ui->canOpenWindowsCheckBox->setChecked(options.value(QLatin1String("Browser/JavaScriptCanOpenWindows")).toBool());
	m_ui->canCloseWindowsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Never"), QLatin1String("disallow"));

	const int canCloseWindowsIndex = m_ui->canCloseWindowsComboBox->findData(options.value(QLatin1String("Browser/JavaScriptCanCloseWindows")).toString());

	m_ui->canCloseWindowsComboBox->setCurrentIndex((canCloseWindowsIndex < 0) ? 0 : canCloseWindowsIndex);

	adjustSize();

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

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		adjustSize();
	}
}

QVariantMap JavaScriptPreferencesDialog::getOptions() const
{
	QVariantMap options;
	options[QLatin1String("Browser/JavaScriptCanChangeWindowGeometry")] = m_ui->canChangeWindowGeometryCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanShowStatusMessages")] = m_ui->canShowStatusMessagesCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanAccessClipboard")] = m_ui->canAccessClipboardCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanDisableContextMenu")] = m_ui->canDisableContextMenuCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanOpenWindows")] = m_ui->canOpenWindowsCheckBox->isChecked();
	options[QLatin1String("Browser/JavaScriptCanCloseWindows")] = m_ui->canCloseWindowsComboBox->currentData().toString();

	return options;
}

}
