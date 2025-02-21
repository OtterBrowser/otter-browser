/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

class WebsitePreferencesDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit WebsitePreferencesDialog(const QString &host, const QVector<QNetworkCookie> &cookies, QWidget *parent = nullptr);
	~WebsitePreferencesDialog();

	QVector<QNetworkCookie> getCookiesToDelete() const;
	QVector<QNetworkCookie> getCookiesToInsert() const;
	QString getHost() const;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void addCookie(const QNetworkCookie &cookie);
	void addNewCookie();
	void removeCookie();
	void cookieProperties();
	void handleButtonClicked(QAbstractButton *button);
	void handleValueChanged();
	void updateCookiesActions();
	void updateValues(bool isChecked = false);

private:
	QVector<QNetworkCookie> m_cookiesToDelete;
	QVector<QNetworkCookie> m_cookiesToInsert;
	bool m_updateOverride;
	Ui::WebsitePreferencesDialog *m_ui;
};

}

#endif
