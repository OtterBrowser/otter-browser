/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_LISTINGNETWORKREPLY_H
#define OTTER_LISTINGNETWORKREPLY_H

#include <QtCore/QMimeType>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class ListingNetworkReply : public QNetworkReply
{
	Q_OBJECT

public:
	explicit ListingNetworkReply(const QNetworkRequest &request, QObject *parent);

protected:
	struct NavigationEntry final
	{
		QString name;
		QUrl url;
	};

	struct ListingEntry final
	{
		enum Type
		{
			UnknownType = 0,
			DriveType,
			DirectoryType,
			FileType
		};

		QString name;
		QUrl url;
		QDateTime timeModified;
		QMimeType mimeType;
		Type type = UnknownType;
		qint64 size = 0;
		bool isSymlink = false;
	};

	QByteArray createListing(const QString &title, const QVector<NavigationEntry> &navigation, const QVector<ListingEntry> &entries);

signals:
	void listingError();
};

}

#endif
