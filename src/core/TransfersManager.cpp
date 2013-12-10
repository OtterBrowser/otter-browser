#include "TransfersManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QMimeDatabase>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
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
	Q_UNUSED(event)

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

	m_replies[reply]->bytesReceivedDifference += (bytesReceived - m_replies[reply]->bytesReceived);
	m_replies[reply]->bytesReceived = bytesReceived;
	m_replies[reply]->bytesTotal = bytesTotal;
}

void TransfersManager::downloadData(QNetworkReply *reply)
{
	if (!reply)
	{
		reply = qobject_cast<QNetworkReply*>(sender());
	}

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	if (m_replies[reply]->state == ErrorTransfer)
	{
		m_replies[reply]->state = RunningTransfer;

		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 206)
		{
			m_replies[reply]->device->reset();
		}
	}

	m_replies[reply]->device->write(reply->readAll());
}

void TransfersManager::downloadFinished(QNetworkReply *reply)
{
	if (!reply)
	{
		reply = qobject_cast<QNetworkReply*>(sender());
	}

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	if (reply->size() > 0)
	{
		m_replies[reply]->device->write(reply->readAll());
	}

	disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), m_instance, SLOT(downloadProgress(qint64,qint64)));
	disconnect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	disconnect(reply, SIGNAL(finished()), m_instance, SLOT(downloadFinished()));

	m_replies[reply]->state = FinishedTransfer;
	m_replies[reply]->finished = QDateTime::currentDateTime();
	m_replies[reply]->bytesReceived = m_replies[reply]->device->size();

	emit m_instance->transferFinished(m_replies[reply]);
	emit m_instance->transferUpdated(m_replies[reply]);

	if (!m_replies[reply]->target.isEmpty())
	{
		m_replies[reply]->device->close();

		m_replies.remove(reply);

		QTimer::singleShot(250, reply, SLOT(deleteLater()));
	}
}

void TransfersManager::downloadError(QNetworkReply::NetworkError error)
{
	Q_UNUSED(error)

	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	TransferInformation *transfer = m_replies[reply];

	stopTransfer(transfer);

	transfer->state = ErrorTransfer;
}

TransfersManager* TransfersManager::getInstance()
{
	return m_instance;
}

TransferInformation* TransfersManager::startTransfer(const QString &source, const QString &target, bool privateTransfer, bool quickTransfer)
{
	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setUrl(QUrl(source));

	return startTransfer(request, target, privateTransfer, quickTransfer);
}

TransferInformation* TransfersManager::startTransfer(const QNetworkRequest &request, const QString &target, bool privateTransfer, bool quickTransfer)
{
	if (!m_networkAccessManager)
	{
		m_networkAccessManager = new NetworkAccessManager(true, false, m_instance);
	}

	return startTransfer(m_networkAccessManager->get(request), target, privateTransfer, quickTransfer);
}

TransferInformation* TransfersManager::startTransfer(QNetworkReply *reply, const QString &target, bool privateTransfer, bool quickTransfer)
{
	if (!reply)
	{
		return NULL;
	}

	reply->setObjectName("transfer");

	TransferInformation *transfer = new TransferInformation();
	transfer->source = reply->url().toString(QUrl::RemovePassword);
	transfer->device = new QTemporaryFile("otter-download-XXXXXX.dat", m_instance);
	transfer->started = QDateTime::currentDateTime();
	transfer->isPrivate = privateTransfer;

	if (!transfer->device->open(QIODevice::ReadWrite))
	{
		transfer->device->deleteLater();

		delete transfer;

		return NULL;
	}

	transfer->state = (reply->isFinished() ? FinishedTransfer : RunningTransfer);

	m_instance->downloadData(reply);

	m_transfers.append(transfer);

	if (transfer->state == RunningTransfer)
	{
		m_replies[reply] = transfer;

		connect(reply, SIGNAL(downloadProgress(qint64,qint64)), m_instance, SLOT(downloadProgress(qint64,qint64)));
		connect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
		connect(reply, SIGNAL(finished()), m_instance, SLOT(downloadFinished()));
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), m_instance, SLOT(downloadError(QNetworkReply::NetworkError)));
	}
	else
	{
		transfer->finished = QDateTime::currentDateTime();
	}

	if (target.isEmpty())
	{
		QFileInfo fileInfo;
		QString fileName;

		if (reply->rawHeaderList().contains("Content-Disposition"))
		{
			QRegExp expression(" filename=\"?([^\"]+)\"?");
			expression.indexIn(QString(reply->rawHeader("Content-Disposition")));

			fileInfo = QFileInfo(expression.cap(1));

			fileName = fileInfo.fileName();
		}

		if (fileName.isEmpty())
		{
			fileInfo = QFileInfo(transfer->source);

			fileName = fileInfo.fileName();
		}

		if (fileName.isEmpty())
		{
			fileName = tr("file");
		}

		if (fileInfo.suffix().isEmpty())
		{
			QString suffix;

			if (reply->header(QNetworkRequest::ContentTypeHeader).isValid())
			{
				suffix = QMimeDatabase().mimeTypeForName(reply->header(QNetworkRequest::ContentTypeHeader).toString()).preferredSuffix();
			}

			if (suffix.isEmpty())
			{
				disconnect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));

				transfer->device->reset();

				suffix = QMimeDatabase().mimeTypeForData(transfer->device).preferredSuffix();

				transfer->device->seek(transfer->device->size());

				connect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
			}

			if (!suffix.isEmpty())
			{
				fileName.append('.');
				fileName.append(suffix);
			}
		}

		QString path;

		if (quickTransfer)
		{
			path = SettingsManager::getValue("Paths/Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString() + '/' + fileName;

			if (QFile::exists(path) && QMessageBox::question(SessionsManager::getActiveWindow(), tr("Question"), tr("File with that name already exists.\nDo you want to overwite it?"), (QMessageBox::Yes | QMessageBox::No)) == QMessageBox::No)
			{
				path = QString();
			}
		}

		do
		{
			if (path.isEmpty())
			{
				path = QFileDialog::getSaveFileName(SessionsManager::getActiveWindow(), tr("Save File"), SettingsManager::getValue("Paths/SaveFile", SettingsManager::getValue("Paths/Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))).toString() + '/' + fileName);
			}

			if (isDownloading(QString(), path))
			{
				if (QMessageBox::warning(SessionsManager::getActiveWindow(), tr("Warning"), tr("Target path is already used by another transfer.\nSelect another one."), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Cancel)
				{
					path = QString();

					break;
				}
			}
			else
			{
				break;
			}
		}
		while (true);

		if (path.isEmpty())
		{
			removeTransfer(transfer, false);

			return NULL;
		}

		SettingsManager::setValue("Paths/SaveFile", QFileInfo(path).dir().canonicalPath());

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

	if (m_replies.contains(reply))
	{
		disconnect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	}

	QIODevice *device = transfer->device;
	device->reset();

	file->write(device->readAll());

	transfer->device = file;

	if (m_replies.contains(reply))
	{
		if (reply->isFinished())
		{
			m_instance->downloadFinished(reply);
		}
		else
		{
			m_instance->downloadData(reply);
		}

		connect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	}

	device->close();
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
	if (!m_transfers.contains(transfer) || m_replies.key(transfer) || transfer->state != ErrorTransfer || !QFile::exists(transfer->target))
	{
		return false;
	}

	QFile *file = new QFile(transfer->target);

	if (!file->open(QIODevice::WriteOnly | QIODevice::Append))
	{
		return false;
	}

	transfer->device = file;
	transfer->started = QDateTime::currentDateTime();

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setUrl(QUrl(transfer->source));
	request.setRawHeader("Range", "bytes=" + QByteArray::number(file->size()) + '-');

	if (!m_networkAccessManager)
	{
		m_networkAccessManager = new NetworkAccessManager(true, false, m_instance);
	}

	QNetworkReply *reply = m_networkAccessManager->get(request);
	reply->setObjectName("transfer");

	m_replies[reply] = transfer;

	m_instance->downloadData(reply);

	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), m_instance, SLOT(downloadProgress(qint64,qint64)));
	connect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	connect(reply, SIGNAL(finished()), m_instance, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), m_instance, SLOT(downloadError(QNetworkReply::NetworkError)));

	m_instance->startUpdates();

	return true;
}

bool TransfersManager::removeTransfer(TransferInformation *transfer, bool keepFile)
{
	if (!transfer || !m_transfers.contains(transfer))
	{
		return false;
	}

	stopTransfer(transfer);

	if (!keepFile && !transfer->target.isEmpty() && QFile::exists(transfer->target))
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

		QTimer::singleShot(250, reply, SLOT(deleteLater()));

		m_replies.remove(reply);
	}

	if (transfer->device)
	{
		transfer->device->close();
		transfer->device->deleteLater();
		transfer->device = NULL;
	}

	transfer->state = ErrorTransfer;
	transfer->finished = QDateTime::currentDateTime();

	emit m_instance->transferStopped(transfer);
	emit m_instance->transferUpdated(transfer);

	return true;
}

bool TransfersManager::isDownloading(const QString &source, const QString &target)
{
	if (source.isEmpty() && target.isEmpty())
	{
		return false;
	}

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->state != RunningTransfer)
		{
			continue;
		}

		if (source.isEmpty() && m_transfers.at(i)->target == target)
		{
			return true;
		}

		if (target.isEmpty() && m_transfers.at(i)->source == source)
		{
			return true;
		}

		if (!source.isEmpty() && !target.isEmpty() && m_transfers.at(i)->source == source && m_transfers.at(i)->target == target)
		{
			return true;
		}
	}

	return false;
}

}
