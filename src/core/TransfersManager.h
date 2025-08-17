/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TRANSFERSMANAGER_H
#define OTTER_TRANSFERSMANAGER_H

#include <QtCore/QFile>
#include <QtCore/QMimeType>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class NetworkManager;

class Transfer : public QObject
{
	Q_OBJECT

public:
	enum TransferOption
	{
		NoOption = 0,
		CanOverwriteOption = 1,
		CanNotifyOption = 2,
		CanAutoDeleteOption = 4,
		CanAskForPathOption = 8,
		IsQuickTransferOption = 16,
		IsPrivateOption = 32,
		HasToOpenAfterFinishOption = 64
	};

	Q_DECLARE_FLAGS(TransferOptions, TransferOption)

	enum TransferState
	{
		UnknownState = 0,
		ErrorState,
		CancelledState,
		RunningState,
		FinishedState
	};

	~Transfer();

	void setHash(QCryptographicHash::Algorithm algorithm, const QByteArray &hash = {});
	virtual void setUpdateInterval(int interval);
	virtual QUrl getSource() const;
	virtual QString getSuggestedFileName();
	virtual QString getTarget() const;
	QIcon getIcon() const;
	virtual QDateTime getTimeStarted() const;
	virtual QDateTime getTimeFinished() const;
	virtual QMimeType getMimeType() const;
	virtual qint64 getSpeed() const;
	virtual qint64 getBytesReceived() const;
	virtual qint64 getBytesTotal() const;
	TransferOptions getOptions() const;
	virtual TransferState getState() const;
	virtual int getRemainingTime() const;
	bool verifyHashes() const;
	bool isArchived() const;

public slots:
	void openTarget() const;
	virtual void cancel();
	virtual void stop();
	void setOpenCommand(const QString &command);
	virtual bool resume();
	virtual bool restart();
	virtual bool setTarget(const QString &target, bool canOverwriteExisting = false);

protected:
	explicit Transfer(TransferOptions options = CanAskForPathOption, QObject *parent = nullptr);
	explicit Transfer(const QSettings &settings, QObject *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	void start(QNetworkReply *reply, const QString &target);

protected slots:
	void markAsStarted();
	void markAsFinished();
	void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void handleDataAvailable();
	void handleDownloadFinished();
	void handleDownloadError(QNetworkReply::NetworkError error);

private:
	QPointer<QNetworkReply> m_reply;
	QPointer<QFile> m_device;
	QUrl m_source;
	QString m_target;
	QString m_openCommand;
	QString m_suggestedFileName;
	QDateTime m_timeStarted;
	QDateTime m_timeFinished;
	QMimeType m_mimeType;
	QHash<QCryptographicHash::Algorithm, QByteArray> m_hashes;
	QQueue<qint64> m_speeds;
	qint64 m_speed;
	qint64 m_bytesStart;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	TransferOptions m_options;
	TransferState m_state;
	int m_updateTimer;
	int m_updateInterval;
	int m_remainingTime;
	bool m_isSelectingPath;
	bool m_isArchived;

signals:
	void progressChanged(qint64 bytesReceived, qint64 bytesTotal);
	void started();
	void finished();
	void changed();
	void stopped();

friend class TransfersManager;
};

class TransfersManager final : public QObject
{
	Q_OBJECT

public:
	struct ActiveTransfersInformation final
	{
		quint64 bytesTotal = 0;
		quint64 bytesReceived = 0;
		uint activeTransfersAmount = 0;
		uint unknownProgressTransfersAmount = 0;
	};

	static void createInstance();
	static void addTransfer(Transfer *transfer);
	static void clearTransfers(int period = 0);
	static TransfersManager* getInstance();
	static Transfer* startTransfer(const QUrl &source, const QString &target = {}, Transfer::TransferOptions options = Transfer::CanAskForPathOption);
	static Transfer* startTransfer(const QNetworkRequest &request, const QString &target = {}, Transfer::TransferOptions options = Transfer::CanAskForPathOption);
	static Transfer* startTransfer(QNetworkReply *reply, const QString &target = {}, Transfer::TransferOptions options = Transfer::CanAskForPathOption);
	static QVector<Transfer*> getTransfers();
	static ActiveTransfersInformation getActiveTransfersInformation();
	static int getRunningTransfersCount();
	static bool removeTransfer(Transfer *transfer, bool keepFile = true);
	static bool isDownloading(const QString &source, const QString &target = {});
	static bool hasRunningTransfers();

protected:
	explicit TransfersManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	void scheduleSave();
	void updateRunningTransfersState();

protected slots:
	void save();

private:
	int m_saveTimer;

	static TransfersManager *m_instance;
	static QVector<Transfer*> m_transfers;
	static QVector<Transfer*> m_privateTransfers;
	static bool m_isInitilized;
	static bool m_hasRunningTransfers;

signals:
	void transferStarted(Transfer *transfer);
	void transferFinished(Transfer *transfer);
	void transferChanged(Transfer *transfer);
	void transferStopped(Transfer *transfer);
	void transferRemoved(Transfer *transfer);
	void transfersChanged();
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::Transfer::TransferOptions)

#endif
