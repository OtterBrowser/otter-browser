/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	enum ResourceType
	{
		OtherType = 0,
		MainFrameType,
		SubFrameType,
		StyleSheetType,
		ScriptType,
		ImageType,
		ObjectType,
		ObjectSubrequestType,
		XmlHttpRequestType
	};

	explicit NetworkManager(bool isPrivate = false, QObject *parent = NULL);

	CookieJar* getCookieJar();

protected:
	virtual QNetworkReply* createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
	void handleOnlineStateChanged(bool isOnline);

private:
	CookieJar *m_cookieJar;
};

}

#endif
