/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TransfersManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QMimeDatabase>
#include <QtCore/QSettings>
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
	QSettings history(SessionsManager::getProfilePath() + QLatin1String("/transfers.ini"), QSettings::IniFormat);
	const QStringList entries = history.childGroups();

	for (int i = 0; i < entries.count(); ++i)
	{
		TransferInformation *transfer = new TransferInformation();
		transfer->source = history.value(QStringLiteral("%1/source").arg(entries.at(i))).toString();
		transfer->target = history.value(QStringLiteral("%1/target").arg(entries.at(i))).toString();
		transfer->started = history.value(QStringLiteral("%1/started").arg(entries.at(i))).toDateTime();
		transfer->finished = history.value(QStringLiteral("%1/finished").arg(entries.at(i))).toDateTime();
		transfer->bytesTotal = history.value(QStringLiteral("%1/bytesTotal").arg(entries.at(i))).toLongLong();
		transfer->bytesReceived = history.value(QStringLiteral("%1/bytesReceived").arg(entries.at(i))).toLongLong();
		transfer->state = ((transfer->bytesReceived > 0 && transfer->bytesTotal == transfer->bytesReceived) ? FinishedTransfer : ErrorTransfer);

		m_transfers.append(transfer);
	}

	connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(save()));
}

TransfersManager::~TransfersManager()
{
	for (int i = (m_transfers.count() - 1); i >= 0; --i)
	{
		delete m_transfers.takeAt(i);
	}
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

	save();

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
		m_updateTimer = startTimer(500);
	}
}

void TransfersManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (!reply || !m_replies.contains(reply))
	{
		return;
	}

	m_replies[reply]->bytesReceivedDifference += (bytesReceived - (m_replies[reply]->bytesReceived - m_replies[reply]->bytesStart));
	m_replies[reply]->bytesReceived = (m_replies[reply]->bytesStart + bytesReceived);
	m_replies[reply]->bytesTotal = (m_replies[reply]->bytesStart + bytesTotal);
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

	TransferInformation *transfer = m_replies[reply];

	if (transfer->state == ErrorTransfer)
	{
		transfer->state = RunningTransfer;

		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 206)
		{
			transfer->device->reset();
		}
	}

	transfer->device->write(reply->readAll());
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

	TransferInformation *transfer = m_replies[reply];

	if (reply->size() > 0)
	{
		transfer->device->write(reply->readAll());
	}

	disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), m_instance, SLOT(downloadProgress(qint64,qint64)));
	disconnect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	disconnect(reply, SIGNAL(finished()), m_instance, SLOT(downloadFinished()));

	transfer->state = FinishedTransfer;
	transfer->finished = QDateTime::currentDateTime();
	transfer->bytesReceived = (transfer->device ? transfer->device->size() : -1);

	if (transfer->bytesTotal <= 0 && transfer->bytesReceived > 0)
	{
		transfer->bytesTotal = transfer->bytesReceived;
	}

	if (transfer->bytesReceived == 0 || transfer->bytesReceived < transfer->bytesTotal)
	{
		transfer->state = ErrorTransfer;
	}

	emit m_instance->transferFinished(transfer);
	emit m_instance->transferUpdated(transfer);

	if (transfer->device && !transfer->device->inherits(QStringLiteral("QTemporaryFile").toLatin1()))
	{
		transfer->device->close();
		transfer->device->deleteLater();
		transfer->device = NULL;

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

void TransfersManager::save()
{
	QSettings history(SessionsManager::getProfilePath() + QLatin1String("/transfers.ini"), QSettings::IniFormat);
	history.clear();

	if (SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool() || !SettingsManager::getValue(QLatin1String("History/RememberDownloads")).toBool())
	{
		return;
	}

	int entry = 1;

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->isPrivate || (m_transfers.at(i)->finished.isValid() && m_transfers.at(i)->finished.daysTo(QDateTime::currentDateTime()) > SettingsManager::getValue(QLatin1String("History/DownloadsLimitPeriod")).toInt()))
		{
			continue;
		}

		history.setValue(QStringLiteral("%1/source").arg(entry), m_transfers.at(i)->source);
		history.setValue(QStringLiteral("%1/target").arg(entry), m_transfers.at(i)->target);
		history.setValue(QStringLiteral("%1/started").arg(entry), m_transfers.at(i)->started);
		history.setValue(QStringLiteral("%1/finished").arg(entry), ((m_transfers.at(i)->finished.isValid() && m_transfers.at(i)->state != RunningTransfer) ? m_transfers.at(i)->finished : QDateTime::currentDateTime()));
		history.setValue(QStringLiteral("%1/bytesTotal").arg(entry), m_transfers.at(i)->bytesTotal);
		history.setValue(QStringLiteral("%1/bytesReceived").arg(entry), m_transfers.at(i)->bytesReceived);

		++entry;
	}

	history.sync();
}

void TransfersManager::clearTransfers(int period)
{
	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->state == FinishedTransfer && (period == 0 || (m_transfers.at(i)->finished.isValid() && m_transfers.at(i)->finished.secsTo(QDateTime::currentDateTime()) > (period * 3600))))
		{
			TransfersManager::removeTransfer(m_transfers.at(i));
		}
	}
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
		m_networkAccessManager = new NetworkAccessManager(true, true, NULL);
		m_networkAccessManager->setParent(m_instance);
	}

	return startTransfer(m_networkAccessManager->get(request), target, privateTransfer, quickTransfer);
}

TransferInformation* TransfersManager::startTransfer(QNetworkReply *reply, const QString &target, bool privateTransfer, bool quickTransfer)
{
	if (!reply)
	{
		return NULL;
	}

	QPointer<QNetworkReply> replyPointer = reply;
	QTemporaryFile temporaryFile(QLatin1String("otter-download-XXXXXX.dat"), m_instance);
	TransferInformation *transfer = new TransferInformation();
	transfer->source = reply->url().toString(QUrl::RemovePassword | QUrl::PreferLocalFile);
	transfer->device = &temporaryFile;
	transfer->started = QDateTime::currentDateTime();
	transfer->isPrivate = privateTransfer;

	if (!transfer->device->open(QIODevice::ReadWrite))
	{
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

		if (reply->rawHeaderList().contains(QStringLiteral("Content-Disposition").toLatin1()))
		{
			fileInfo = QFileInfo(QRegularExpression(QLatin1String(" filename=\"?([^\"]+)\"?")).match(QString(reply->rawHeader(QStringLiteral("Content-Disposition").toLatin1()))).captured(1));

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

		if (!quickTransfer && !SettingsManager::getValue(QLatin1String("Browser/AlwaysAskWhereToSaveDownload")).toBool())
		{
			quickTransfer = true;
		}

		if (quickTransfer)
		{
			path = SettingsManager::getValue(QLatin1String("Paths/Downloads")).toString() + QLatin1Char('/') + fileName;

			if (QFile::exists(path) && QMessageBox::question(SessionsManager::getActiveWindow(), tr("Question"), tr("File with that name already exists.\nDo you want to overwite it?"), (QMessageBox::Yes | QMessageBox::No)) == QMessageBox::No)
			{
				path = QString();
			}
		}

		do
		{
			if (path.isEmpty())
			{
				path = QFileDialog::getSaveFileName(SessionsManager::getActiveWindow(), tr("Save File"), SettingsManager::getValue(QLatin1String("Paths/SaveFile")).toString() + '/' + fileName);
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
			transfer->device = NULL;

			m_replies.remove(reply);

			removeTransfer(transfer, false);

			if (replyPointer)
			{
				reply->abort();
			}

			return NULL;
		}

		SettingsManager::setValue(QLatin1String("Paths/SaveFile"), QFileInfo(path).dir().canonicalPath());

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
		if (transfer->state == RunningTransfer && replyPointer)
		{
			disconnect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
		}
		else
		{
			m_replies.remove(reply);
		}
	}

	temporaryFile.reset();

	file->write(temporaryFile.readAll());

	transfer->device = file;

	if (m_replies.contains(reply) && replyPointer)
	{
		if (reply->isFinished())
		{
			m_instance->downloadFinished(reply);

			transfer->device = NULL;
		}
		else
		{
			m_instance->downloadData(reply);
		}

		connect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	}
	else
	{
		transfer->device = NULL;
	}

	temporaryFile.close();

	if (transfer->state == FinishedTransfer)
	{
		if (transfer->bytesTotal <= 0 && transfer->bytesReceived > 0)
		{
			transfer->bytesTotal = transfer->bytesReceived;
		}

		if (transfer->bytesReceived == 0 || transfer->bytesReceived < transfer->bytesTotal)
		{
			transfer->state = ErrorTransfer;
		}
	}

	emit m_instance->transferStarted(transfer);

	if (m_replies.contains(reply) && replyPointer)
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
	transfer->bytesStart = file->size();

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setUrl(QUrl(transfer->source));
	request.setRawHeader(QStringLiteral("Range").toLatin1(), QStringLiteral("bytes=%1-").arg(file->size()).toLatin1());

	if (!m_networkAccessManager)
	{
		m_networkAccessManager = new NetworkAccessManager(true, true, NULL);
		m_networkAccessManager->setParent(m_instance);
	}

	QNetworkReply *reply = m_networkAccessManager->get(request);

	m_replies[reply] = transfer;

	m_instance->downloadData(reply);

	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), m_instance, SLOT(downloadProgress(qint64,qint64)));
	connect(reply, SIGNAL(readyRead()), m_instance, SLOT(downloadData()));
	connect(reply, SIGNAL(finished()), m_instance, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), m_instance, SLOT(downloadError(QNetworkReply::NetworkError)));

	m_instance->startUpdates();

	return true;
}

bool TransfersManager::restartTransfer(TransferInformation *transfer)
{
	if (!transfer || !m_transfers.contains(transfer))
	{
		return false;
	}

	stopTransfer(transfer);

	QFile *file = new QFile(transfer->target);

	if (!file->open(QIODevice::WriteOnly))
	{
		return false;
	}

	transfer->device = file;
	transfer->started = QDateTime::currentDateTime();
	transfer->bytesStart = 0;

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setUrl(QUrl(transfer->source));

	if (!m_networkAccessManager)
	{
		m_networkAccessManager = new NetworkAccessManager(true, true, NULL);
		m_networkAccessManager->setParent(m_instance);
	}

	QNetworkReply *reply = m_networkAccessManager->get(request);

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
