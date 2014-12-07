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

#ifndef OTTER_NETWORKMANAGER_H
#define OTTER_NETWORKMANAGER_H

#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

class CookieJar;

class NetworkManager : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit NetworkManager(bool isPrivate = false, QObject *parent = NULL);

	CookieJar* getCookieJar();

protected:
	virtual QNetworkReply* createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	virtual void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	virtual void handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
	virtual void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
	CookieJar *m_cookieJar;
};

}

#endif
