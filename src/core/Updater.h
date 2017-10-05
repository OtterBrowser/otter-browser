/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_UPDATER_H
#define OTTER_UPDATER_H

#include "UpdateChecker.h"

#include <QtCore/QObject>

namespace Otter
{

class Transfer;

class Updater final : public QObject
{
	Q_OBJECT

public:
	explicit Updater(const UpdateChecker::UpdateInformation &information, QObject *parent = nullptr);

	static void clearUpdate();
	static QString getScriptPath();
	static bool installUpdate();
	static bool isReadyToInstall(QString path = {});

protected:
	Transfer* downloadFile(const QUrl &url, const QString &path);

protected slots:
	void handleTransferFinished();
	void updateProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
	Transfer *m_transfer;
	int m_transfersCount;
	bool m_transfersSuccessful;

signals:
	void progress(int percentage);
	void finished(bool isSuccess);
};

}

#endif
