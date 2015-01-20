/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TRANSFERSMANAGER_H
#define OTTER_TRANSFERSMANAGER_H

#include <QtNetwork/QNetworkReply>

namespace Otter
{

class Transfer;

class TransfersManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void clearTransfers(int period = 0);
	static TransfersManager* getInstance();
	static Transfer* startTransfer(const QUrl &source, const QString &target = QString(), bool quickTransfer = false, bool isPrivate = false);
	static Transfer* startTransfer(const QNetworkRequest &request, const QString &target = QString(), bool quickTransfer = false, bool isPrivate = false);
	static Transfer* startTransfer(QNetworkReply *reply, const QString &target = QString(), bool quickTransfer = false, bool isPrivate = false);
	static QString getSavePath(const QString &fileName, QString path = QString());
	static QVector<Transfer*> getTransfers();
	static bool removeTransfer(Transfer *transfer, bool keepFile = true);
	static bool isDownloading(const QString &source, const QString &target = QString());

protected:
	explicit TransfersManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void scheduleSave();
	static void addTransfer(Transfer *transfer);

protected slots:
	void save();
	void transferStarted();
	void transferFinished();
	void transferChanged();
	void transferStopped();

private:
	int m_saveTimer;

	static TransfersManager *m_instance;
	static QVector<Transfer*> m_transfers;
	static bool m_initilized;

signals:
	void transferStarted(Transfer *transfer);
	void transferFinished(Transfer *transfer);
	void transferChanged(Transfer *transfer);
	void transferStopped(Transfer *transfer);
	void transferRemoved(Transfer *transfer);
};

}

#endif
