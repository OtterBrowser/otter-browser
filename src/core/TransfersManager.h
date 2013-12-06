#ifndef OTTER_TRANSFERSMANAGER_H
#define OTTER_TRANSFERSMANAGER_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>

namespace Otter
{

struct TransferInformation
{
	QString source;
	QString target;
	QDateTime started;
	QDateTime finished;
	qint64 speed;
	qint64 bytesReceived;
	qint64 bytesTotal;

	TransferInformation() : speed(0), bytesReceived(0), bytesTotal(-1) {}
};

class TransfersManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static TransfersManager* getInstance();
	static TransferInformation* startTransfer(const QString &source, const QString &target = QString());
	static QList<TransferInformation*> getTransfers();
	static bool restartTransfer(TransferInformation *transfer);
	static bool resumeTransfer(TransferInformation *transfer);
	static bool removeTransfer(TransferInformation *transfer, bool keepFile = true);
	static bool stopTransfer(TransferInformation *transfer);

private:
	explicit TransfersManager(QObject *parent = NULL);

	static TransfersManager *m_instance;
	static QList<TransferInformation*> m_transfers;

signals:
	void transferStarted(TransferInformation *transfer);
	void transferFinished(TransferInformation *transfer);
	void transferUpdated(TransferInformation *transfer);
};

}

#endif
