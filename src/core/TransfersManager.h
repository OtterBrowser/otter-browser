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

#include "Transfer.h"

namespace Otter
{

class TransfersManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void addTransfer(Transfer *transfer);
	static void clearTransfers(int period = 0);
	static TransfersManager* getInstance();
	static Transfer* startTransfer(const QUrl &source, const QString &target = QString(), Transfer::TransferOptions options = Transfer::CanAskForPathOption);
	static Transfer* startTransfer(const QNetworkRequest &request, const QString &target = QString(), Transfer::TransferOptions options = Transfer::CanAskForPathOption);
	static Transfer* startTransfer(QNetworkReply *reply, const QString &target = QString(), Transfer::TransferOptions options = Transfer::CanAskForPathOption);
	static QString getSavePath(const QString &fileName, QString path = QString(), bool forceAsk = false);
	static QList<Transfer*> getTransfers();
	static bool removeTransfer(Transfer *transfer, bool keepFile = true);
	static bool isDownloading(const QString &source, const QString &target = QString());

protected:
	explicit TransfersManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void scheduleSave();

protected slots:
	void save();
	void transferStarted();
	void transferFinished();
	void transferChanged();
	void transferStopped();

private:
	int m_saveTimer;

	static TransfersManager *m_instance;
	static QList<Transfer*> m_transfers;
	static QList<Transfer*> m_privateTransfers;
	static bool m_isInitilized;

signals:
	void transferStarted(Transfer *transfer);
	void transferFinished(Transfer *transfer);
	void transferChanged(Transfer *transfer);
	void transferStopped(Transfer *transfer);
	void transferRemoved(Transfer *transfer);
};

}

#endif
