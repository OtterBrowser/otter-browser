/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_LOCALLISTINGNETWORKREPLY_H
#define OTTER_LOCALLISTINGNETWORKREPLY_H

#include "ListingNetworkReply.h"

namespace Otter
{

class LocalListingNetworkReply final : public ListingNetworkReply
{
	Q_OBJECT

public:
	explicit LocalListingNetworkReply(const QNetworkRequest &request, QObject *parent);

	qint64 bytesAvailable() const override;
	qint64 readData(char *data, qint64 maxSize) override;
	bool isSequential() const override;

public slots:
	void abort() override;

private:
	QByteArray m_content;
	qint64 m_offset;
};

}

#endif
