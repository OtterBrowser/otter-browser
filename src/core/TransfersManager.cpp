#include "TransfersManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QBuffer>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

TransfersManager* TransfersManager::m_instance = NULL;
NetworkAccessManager* TransfersManager::m_networkAccessManager = NULL;
QHash<QNetworkReply*, TransferInformation*> TransfersManager::m_replies;
QList<TransferInformation*> TransfersManager::m_transfers;

TransfersManager::TransfersManager(QObject *parent) : QObject(parent),
	m_updateTimer(0)
{
}

void TransfersManager::createInstance(QObject *parent)
{
	m_instance = new TransfersManager(parent);
}

void TransfersManager::timerEvent(QTimerEvent *event)
{
	QHash<QNetworkReply*, TransferInformation*>::iterator iterator;

	for (iterator = m_replies.begin(); iterator != m_replies.end(); ++iterator)
	{
		iterator.value()->speed = (iterator.value()->bytesReceivedDifference * 2);
		iterator.value()->bytesReceivedDifference = 0;

		emit m_instance->transferUpdated(iterator.value());
	}

	if (m_replies.isEmpty())
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;
	}
}

void TransfersManager::startUpdates()
{
	if (m_updateTimer == 0)
	{
		m_updateTimer = startTimer(250);
	}
}

void TransfersManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	m_replies[reply]->device->write(reply->readAll());
	m_replies[reply]->bytesReceivedDifference += (bytesReceived - m_replies[reply]->bytesReceived);
	m_replies[reply]->bytesReceived = bytesReceived;
	m_replies[reply]->bytesTotal = bytesTotal;
}

void TransfersManager::downloadFinished(QNetworkReply *reply)
{
	if (!reply)
	{
		reply = qobject_cast<QNetworkReply*>(sender());
	}

	if (!reply)
	{
		return;
	}

	m_replies[reply]->device->write(reply->readAll());

	emit m_instance->transferFinished(m_replies[reply]);

	m_replies.remove(reply);

	reply->deleteLater();
}

TransfersManager *TransfersManager::getInstance()
{
	return m_instance;
}

TransferInformation *TransfersManager::startTransfer(const QString &source, const QString &target)
{
	QNetworkRequest request;
	request.setUrl(QUrl(source));

	return startTransfer(request, target);
}

TransferInformation *TransfersManager::startTransfer(const QNetworkRequest &request, const QString &target)
{
	if (!m_networkAccessManager)
	{
		m_networkAccessManager = new NetworkAccessManager(true, m_instance);
	}

	return startTransfer(m_networkAccessManager->get(request), target);
}

TransferInformation* TransfersManager::startTransfer(QNetworkReply *reply, const QString &target)
{
	if (!reply)
	{
		return NULL;
	}

	TransferInformation *transfer = new TransferInformation();
	transfer->source = reply->url().toString(QUrl::RemovePassword);
	transfer->device = new QBuffer(m_instance);
	transfer->device->open(QIODevice::ReadWrite);
	transfer->device->write(reply->readAll());

	m_transfers.append(transfer);

	if (!reply->isFinished())
	{
		m_replies[reply] = transfer;

		connect(reply, SIGNAL(downloadProgress(qint64,qint64)), m_instance, SLOT(downloadProgress(qint64,qint64)));
		connect(reply, SIGNAL(finished()), m_instance, SLOT(downloadFinished()));
	}

	if (target.isEmpty())
	{
		const QString path = QFileDialog::getSaveFileName(SessionsManager::getActiveWindow(), tr("Save File"), SettingsManager::getValue("Paths/SaveFile", SettingsManager::getValue("Paths/Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))).toString() + '/' + QFileInfo(transfer->source).fileName());

		if (path.isEmpty())
		{
			removeTransfer(transfer, false);

			return NULL;
		}

		transfer->target = path;
	}
	else
	{
		transfer->target = QFileInfo(target).canonicalFilePath();
	}

	if (!target.isEmpty() && QFile::exists(transfer->target) && QMessageBox::question(SessionsManager::getActiveWindow(), tr("Question"), tr("File with the same name already exists.\nDo you want to overwrite it?\n\n%1").arg(transfer->target), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel)
	{
		removeTransfer(transfer, false);

		return NULL;
	}

	QFile *file = new QFile(transfer->target);

	if (!file->open(QIODevice::WriteOnly))
	{
		removeTransfer(transfer, false);

		return NULL;
	}

	QIODevice *device = transfer->device;
	device->reset();

	file->write(device->readAll());

	transfer->device = file;

	device->deleteLater();

	emit m_instance->transferStarted(transfer);

	if (m_replies.contains(reply))
	{
		m_instance->startUpdates();
	}
	else
	{
		file->close();
		file->deleteLater();

		emit m_instance->transferFinished(transfer);
	}

	return transfer;
}

QList<TransferInformation*> TransfersManager::getTransfers()
{
	return m_transfers;
}

bool TransfersManager::resumeTransfer(TransferInformation *transfer)
{
	Q_UNUSED(transfer)

//TODO

	return false;
}

bool TransfersManager::removeTransfer(TransferInformation *transfer, bool keepFile)
{
	if (!transfer || !m_transfers.contains(transfer))
	{
		return false;
	}

	stopTransfer(transfer);

	if (!keepFile)
	{
		QFile::remove(transfer->target);
	}

	m_transfers.removeAll(transfer);

	emit m_instance->transferRemoved(transfer);

	delete transfer;

	return true;
}

bool TransfersManager::stopTransfer(TransferInformation *transfer)
{
	QNetworkReply *reply = m_replies.key(transfer);

	if (reply)
	{
		reply->abort();
		reply->deleteLater();

		m_replies.remove(reply);
	}

	transfer->device->close();
	transfer->device->deleteLater();
	transfer->device = NULL;

	emit m_instance->transferStopped(transfer);

	return true;
}

}
