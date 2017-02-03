/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ProxyPropertiesDialog.h"

#include "ui_ProxyPropertiesDialog.h"

namespace Otter
{

ProxyPropertiesDialog::ProxyPropertiesDialog(const ProxyDefinition &proxy, QWidget *parent) : Dialog(parent),
	m_proxy(proxy),
	m_ui(new Ui::ProxyPropertiesDialog)
{
	m_ui->setupUi(this);

///TODO

	setWindowTitle(proxy.identifier.isEmpty() ? tr("Add Proxy") : tr ("Edit Proxy"));
}

ProxyPropertiesDialog::~ProxyPropertiesDialog()
{
	delete m_ui;
}

void ProxyPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

ProxyDefinition ProxyPropertiesDialog::getProxy() const
{
	ProxyDefinition proxy;

///TODO

	return proxy;
}

}
