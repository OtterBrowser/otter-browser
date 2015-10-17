/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "Transfer.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "TransfersManager.h"
#include "Utils.h"
#include "../ui/MainWindow.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>

namespace Otter
{

Transfer::Transfer(QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(NULL),
	m_device(NULL),
	m_speed(0),
	m_bytesStart(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_options(NoOption),
	m_state(UnknownState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_isSelectingPath(false)
{
}

Transfer::Transfer(const QSettings &settings, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(NULL),
	m_device(NULL),
	m_source(settings.value(QLatin1String("source")).toUrl()),
	m_target(settings.value(QLatin1String("target")).toString()),
	m_timeStarted(settings.value(QLatin1String("timeStarted")).toDateTime()),
	m_timeFinished(settings.value(QLatin1String("timeFinished")).toDateTime()),
	m_mimeType(QMimeDatabase().mimeTypeForFile(m_target)),
	m_speed(0),
	m_bytesStart(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(settings.value(QLatin1String("bytesReceived")).toLongLong()),
	m_bytesTotal(settings.value(QLatin1String("bytesTotal")).toLongLong()),
	m_options(NoOption),
	m_state((m_bytesReceived > 0 && m_bytesTotal == m_bytesReceived) ? FinishedState : ErrorState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_isSelectingPath(false)
{
}

Transfer::Transfer(const QUrl &source, const QString &target, TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(NULL),
	m_device(NULL),
	m_source(source),
	m_target(target),
	m_speed(0),
	m_bytesStart(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_options(options),
	m_state(UnknownState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_isSelectingPath(false)
{
	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setUrl(QUrl(source));

	start(NetworkManagerFactory::getNetworkManager()->get(request), target);
}

Transfer::Transfer(const QNetworkRequest &request, const QString &target, TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(NULL),
	m_device(NULL),
	m_source(request.url()),
	m_target(target),
	m_speed(0),
	m_bytesStart(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_options(options),
	m_state(UnknownState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_isSelectingPath(false)
{
	start(NetworkManagerFactory::getNetworkManager()->get(request), target);
}

Transfer::Transfer(QNetworkReply *reply, const QString &target, TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(reply),
	m_source(m_reply->url().adjusted(QUrl::RemovePassword | QUrl::PreferLocalFile)),
	m_target(target),
	m_speed(0),
	m_bytesStart(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_options(options),
	m_state(UnknownState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_isSelectingPath(false)
{
	start(reply, target);
}

Transfer::~Transfer()
{
	if (m_options.testFlag(HasToOpenAfterFinishOption) && QFile::exists(m_target))
	{
		QFile::remove(m_target);
	}
}

void Transfer::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		const qint64 oldSpeed = m_speed;

		m_speed = (m_bytesReceivedDifference * 2);
		m_bytesReceivedDifference = 0;

		if (m_speed != oldSpeed)
		{
			emit changed();
		}
	}
}

void Transfer::start(QNetworkReply *reply, const QString &target)
{
	if (!reply)
	{
		m_state = ErrorState;

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}

		return;
	}

	m_reply = reply;
	m_mimeType = QMimeDatabase().mimeTypeForName(m_reply->header(QNetworkRequest::ContentTypeHeader).toString());

	QString temporaryFileName = getSuggestedFileName();

	if (temporaryFileName.isEmpty())
	{
		temporaryFileName = QLatin1String("otter-download-XXXXXX.") + m_mimeType.preferredSuffix();
	}
	else if (temporaryFileName.contains(QLatin1Char('.')))
	{
		QStringList parts = temporaryFileName.split(QLatin1Char('.'));
		parts[parts.count() - 2].append(QLatin1String("-XXXXXX"));

		temporaryFileName = parts.join(QLatin1Char('.'));
	}

	m_device = new QTemporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + temporaryFileName, this);
	m_timeStarted = QDateTime::currentDateTime();
	m_bytesTotal = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();

	if (!m_device->open(QIODevice::ReadWrite))
	{
		m_state = ErrorState;

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}

		return;
	}

	m_target = m_device->fileName();
	m_state = (m_reply->isFinished() ? FinishedState : RunningState);

	downloadData();

	const bool isRunning = (m_state == RunningState);

	if (isRunning)
	{
		connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
		connect(m_reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
	}
	else
	{
		m_timeFinished = QDateTime::currentDateTime();

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}
	}

	m_device->reset();

	m_mimeType = QMimeDatabase().mimeTypeForData(m_device);

	m_device->seek(m_device->size());

	downloadData();

	if (isRunning)
	{
		connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadData()));
	}

	QString finalTarget;

	if (target.isEmpty())
	{
		QString path;
		QString fileName = getSuggestedFileName();

		if (!m_options.testFlag(IsQuickTransferOption) && !SettingsManager::getValue(QLatin1String("Browser/AlwaysAskWhereToSaveDownload")).toBool())
		{
			m_options |= IsQuickTransferOption;
		}

		if (m_options.testFlag(IsQuickTransferOption))
		{
			path = SettingsManager::getValue(QLatin1String("Paths/Downloads")).toString() + QDir::separator() + fileName;

			if (QFile::exists(path) && QMessageBox::question(SessionsManager::getActiveWindow(), tr("Question"), tr("File with that name already exists.\nDo you want to overwite it?"), (QMessageBox::Yes | QMessageBox::No)) == QMessageBox::No)
			{
				path = QString();
			}
		}

		if (m_options.testFlag(CanAskForPathOption))
		{
			m_isSelectingPath = true;

			path = TransfersManager::getSavePath(fileName, path);

			m_isSelectingPath = false;

			if (path.isEmpty())
			{
				if (m_reply)
				{
					m_reply->abort();
				}

				m_device = NULL;

				cancel();

				return;
			}
		}

		finalTarget = QDir::toNativeSeparators(path);
	}
	else
	{
		finalTarget = QFileInfo(QDir::toNativeSeparators(target)).absoluteFilePath();
	}

	if (!finalTarget.isEmpty())
	{
		setTarget(finalTarget);
	}

	if (m_state == FinishedState)
	{
		if (m_bytesTotal <= 0 && m_bytesReceived > 0)
		{
			m_bytesTotal = m_bytesReceived;
		}

		if (m_bytesReceived == 0 || m_bytesReceived < m_bytesTotal)
		{
			m_state = ErrorState;

			if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
			{
				deleteLater();
			}
		}
		else
		{
			m_mimeType = QMimeDatabase().mimeTypeForFile(m_target);
		}
	}
}

void Transfer::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_bytesReceivedDifference += (bytesReceived - (m_bytesReceived - m_bytesStart));
	m_bytesReceived = (m_bytesStart + bytesReceived);
	m_bytesTotal = (m_bytesStart + bytesTotal);

	emit progressChanged(bytesReceived, bytesTotal);
}

void Transfer::downloadData()
{
	if (!m_reply)
	{
		return;
	}

	if (m_state == ErrorState)
	{
		m_state = RunningState;

		if (m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid() && m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 206)
		{
			m_device->reset();
		}
	}

	m_device->write(m_reply->readAll());
	m_device->seek(m_device->size());
}

void Transfer::downloadFinished()
{
	if (!m_reply)
	{
		if (m_device && !m_device->inherits(QStringLiteral("QTemporaryFile").toLatin1()))
		{
			m_device->close();
			m_device->deleteLater();
			m_device = NULL;
		}

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}

		return;
	}

	if (!m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isNull())
	{
		m_source = m_source.resolved(m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());

		restart();

		return;
	}

	if (m_updateTimer != 0)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;
	}

	if (m_reply->size() > 0)
	{
		m_device->write(m_reply->readAll());
	}

	disconnect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
	disconnect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadData()));
	disconnect(m_reply, SIGNAL(finished()), this, SLOT(downloadFinished()));

	m_bytesReceived = (m_device ? m_device->size() : -1);

	if (m_bytesTotal <= 0 && m_bytesReceived > 0)
	{
		m_bytesTotal = m_bytesReceived;
	}

	if (m_bytesReceived == 0 || m_bytesReceived < m_bytesTotal)
	{
		m_state = ErrorState;
	}
	else
	{
		m_state = FinishedState;
		m_timeFinished = QDateTime::currentDateTime();
		m_mimeType = QMimeDatabase().mimeTypeForFile(m_target);
	}

	emit finished();
	emit changed();

	if (m_device && (m_options.testFlag(HasToOpenAfterFinishOption) || !m_device->inherits(QStringLiteral("QTemporaryFile").toLatin1())))
	{
		m_device->close();
		m_device->deleteLater();
		m_device = NULL;

		if (m_reply)
		{
			QTimer::singleShot(250, m_reply, SLOT(deleteLater()));
		}
	}

	if (m_state == FinishedState && m_options.testFlag(HasToOpenAfterFinishOption))
	{
		openTarget();
	}

	if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
	{
		deleteLater();
	}
}

void Transfer::downloadError(QNetworkReply::NetworkError error)
{
	Q_UNUSED(error)

	stop();

	m_state = ErrorState;

	if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
	{
		deleteLater();
	}
}

void Transfer::openTarget()
{
	Utils::runApplication(m_openCommand, QUrl::fromLocalFile(getTarget()));
}

void Transfer::cancel()
{
	m_state = CancelledState;

	if (m_reply)
	{
		m_reply->abort();

		QTimer::singleShot(250, m_reply, SLOT(deleteLater()));
	}

	if (m_device)
	{
		m_device->remove();
	}

	stop();

	if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
	{
		deleteLater();
	}
}

void Transfer::stop()
{
	if (m_updateTimer != 0)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;
	}

	if (m_reply)
	{
		m_reply->abort();

		QTimer::singleShot(250, m_reply, SLOT(deleteLater()));
	}

	if (m_device && !m_device->inherits(QStringLiteral("QTemporaryFile").toLatin1()))
	{
		m_device->close();
		m_device->deleteLater();
		m_device = NULL;
	}

	if (m_state == RunningState)
	{
		m_state = ErrorState;

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}
	}

	emit stopped();
	emit changed();
}

void Transfer::setOpenCommand(const QString &command)
{
	m_openCommand = command;

	m_options |= HasToOpenAfterFinishOption;

	QTemporaryFile *file = qobject_cast<QTemporaryFile*>(m_device);

	if (file)
	{
		file->setAutoRemove(false);
	}

	if (m_state == FinishedState)
	{
		if (file)
		{
			file->close();
			file->deleteLater();

			m_device = NULL;
		}

		openTarget();
	}
}

void Transfer::setUpdateInterval(int interval)
{
	m_updateInterval = interval;

	if (m_updateTimer != 0)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;
	}

	if (interval > 0 && m_updateInterval > 0 && m_state == Transfer::RunningState)
	{
		m_updateTimer = startTimer(m_updateInterval);
	}
}

QUrl Transfer::getSource() const
{
	return m_source;
}

QString Transfer::getSuggestedFileName()
{
	if (!m_reply)
	{
		return m_suggestedFileName;
	}

	QUrl url;
	QString fileName;

	if (m_reply->hasRawHeader(QStringLiteral("Content-Disposition").toLatin1()))
	{
		url = QUrl(QRegularExpression(QLatin1String(" filename=\"?([^\"]+)\"?")).match(QString(m_reply->rawHeader(QStringLiteral("Content-Disposition").toLatin1()))).captured(1));

		fileName = url.fileName();
	}

	if (fileName.isEmpty())
	{
		url = m_source;

		fileName = url.fileName();
	}

	if (fileName.isEmpty())
	{
		fileName = tr("file");
	}

	if (QFileInfo(fileName).suffix().isEmpty())
	{
		QString suffix;

		if (m_reply->header(QNetworkRequest::ContentTypeHeader).isValid())
		{
			suffix = m_mimeType.preferredSuffix();
		}

		if (!suffix.isEmpty())
		{
			fileName.append(QLatin1Char('.'));
			fileName.append(suffix);
		}
	}

	m_suggestedFileName = fileName;

	return fileName;
}

QString Transfer::getTarget() const
{
	return m_target;
}

QDateTime Transfer::getTimeStarted() const
{
	return m_timeStarted;
}

QDateTime Transfer::getTimeFinished() const
{
	return m_timeFinished;
}

QMimeType Transfer::getMimeType() const
{
	return m_mimeType;
}

qint64 Transfer::getSpeed() const
{
	return m_speed;
}

qint64 Transfer::getBytesReceived() const
{
	return m_bytesReceived;
}

qint64 Transfer::getBytesTotal() const
{
	return m_bytesTotal;
}

Transfer::TransferOptions Transfer::getOptions() const
{
	return m_options;
}

Transfer::TransferState Transfer::getState() const
{
	return m_state;
}

bool Transfer::resume()
{
	if (m_state != ErrorState || !QFile::exists(m_target))
	{
		return false;
	}

	if (m_bytesTotal == 0)
	{
		return restart();
	}

	QFile *file = new QFile(m_target);

	if (!file->open(QIODevice::WriteOnly | QIODevice::Append))
	{
		file->deleteLater();

		return false;
	}

	m_state = RunningState;
	m_device = file;
	m_timeStarted = QDateTime::currentDateTime();
	m_timeFinished = QDateTime();
	m_bytesStart = file->size();

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setRawHeader(QStringLiteral("Range").toLatin1(), QStringLiteral("bytes=%1-").arg(file->size()).toLatin1());
	request.setUrl(m_source);

	m_reply = NetworkManagerFactory::getNetworkManager()->get(request);

	downloadData();

	connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
	connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadData()));
	connect(m_reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));

	if (m_updateTimer == 0 && m_updateInterval > 0)
	{
		m_updateTimer = startTimer(m_updateInterval);
	}

	return true;
}

bool Transfer::restart()
{
	stop();

	QFile *file = new QFile(m_target);

	if (!file->open(QIODevice::WriteOnly))
	{
		file->deleteLater();

		return false;
	}

	m_state = RunningState;
	m_device = file;
	m_timeStarted = QDateTime::currentDateTime();
	m_timeFinished = QDateTime();
	m_bytesStart = 0;

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setUrl(QUrl(m_source));

	m_reply = NetworkManagerFactory::getNetworkManager()->get(request);

	downloadData();

	connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
	connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadData()));
	connect(m_reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));

	if (m_updateTimer == 0 && m_updateInterval > 0)
	{
		m_updateTimer = startTimer(m_updateInterval);
	}

	return true;
}

bool Transfer::setTarget(const QString &target)
{
	if (m_target == target)
	{
		return false;
	}

	QString mutableTarget(target);

	if (QFile::exists(target) && !m_options.testFlag(CanOverwriteOption))
	{
		const QMessageBox::StandardButton result = QMessageBox::question(SessionsManager::getActiveWindow(), tr("Question"), tr("File with the same name already exists.\nDo you want to overwrite it?\n\n%1").arg(target), (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel));

		if (result == QMessageBox::No)
		{
			QFileInfo information(target);
			const QString path = TransfersManager::getSavePath(information.fileName(), information.path(), true);

			if (path.isEmpty())
			{
				cancel();

				return false;
			}

			mutableTarget = path;
		}
		else if (result == QMessageBox::Cancel)
		{
			cancel();

			return false;
		}
	}

	if (!m_device)
	{
		if (m_state != FinishedState)
		{
			return false;
		}

		const bool success = QFile::rename(m_target, mutableTarget);

		if (success)
		{
			m_target = mutableTarget;
		}

		return success;
	}

	QFile *file = new QFile(mutableTarget, this);

	if (!file->open(QIODevice::WriteOnly))
	{
		m_state = ErrorState;

		file->deleteLater();

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}

		return false;
	}

	m_target = mutableTarget;

	if (m_device)
	{
		if (m_reply && m_state == RunningState)
		{
			disconnect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadData()));
		}

		m_device->reset();

		file->write(m_device->readAll());

		m_device->close();
		m_device->deleteLater();
	}

	m_device = file;

	downloadData();

	if (!m_reply || m_reply->isFinished())
	{
		downloadFinished();
	}
	else
	{
		connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadData()));
	}

	return false;
}

}
