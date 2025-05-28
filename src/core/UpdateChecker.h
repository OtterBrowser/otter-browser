/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_UPDATECHECKER_H
#define OTTER_UPDATECHECKER_H

#include "SessionsManager.h"

namespace Otter
{

class UpdateChecker final : public QObject
{
	Q_OBJECT

public:
	struct UpdateInformation final
	{
		QString channel;
		QString version;
		QUrl detailsUrl;
		QUrl scriptUrl;
		QUrl fileUrl;
		bool isAvailable = false;
	};

	explicit UpdateChecker(QObject *parent = nullptr, bool isInBackground = true);

signals:
	void finished(const QVector<UpdateChecker::UpdateInformation> &availableUpdates, int latestVersionIndex);
};

}

Q_DECLARE_METATYPE(Otter::UpdateChecker::UpdateInformation)

#endif
