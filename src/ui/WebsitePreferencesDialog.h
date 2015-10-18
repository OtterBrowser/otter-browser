/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_WEBSITEPREFERENCESDIALOG_H
#define OTTER_WEBSITEPREFERENCESDIALOG_H

#include "Dialog.h"

#include <QtNetwork/QNetworkCookie>
#include <QtWidgets/QAbstractButton>

namespace Otter
{

namespace Ui
{
	class WebsitePreferencesDialog;
}

class WebsitePreferencesDialog : public Dialog
{
	Q_OBJECT

public:
	explicit WebsitePreferencesDialog(const QUrl &url, const QList<QNetworkCookie> &cookies, QWidget *parent = NULL);
	~WebsitePreferencesDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void buttonClicked(QAbstractButton *button);
	void updateContentBlockingProfile(const QString &profile);
	void updateValues(bool checked = false);
	void valueChanged();

private:
	bool m_updateOverride;
	Ui::WebsitePreferencesDialog *m_ui;
};

}

#endif
