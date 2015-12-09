/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_FTPLISTINGNETWORKREPLY_H
#define OTTER_FTPLISTINGNETWORKREPLY_H

#include "3rdparty/qtftp/qftp.h"
#include "3rdparty/qtftp/qurlinfo.h"

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class QtWebKitFtpListingNetworkReply : public QNetworkReply
{
	Q_OBJECT

public:
	explicit QtWebKitFtpListingNetworkReply(const QNetworkRequest &request, QObject *parent);

	qint64 bytesAvailable() const;
	qint64 readData(char *data, qint64 maxSize);
	bool isSequential() const;

public slots:
	void abort();

protected slots:
	void processCommand(int command, bool isError);
	void addEntry(const QUrlInfo &urlInfo);
	void processData();

private:
	QFtp *m_ftp;
	QByteArray m_content;
	QList<QUrlInfo> m_directories;
	QList<QUrlInfo> m_files;
	qint64 m_offset;
};

}

#endif
