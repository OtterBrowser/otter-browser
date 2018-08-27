/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TransfersManager.h"
#include "Application.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "NotificationsManager.h"
#include "SessionsManager.h"
#include "Utils.h"
#include "../ui/MainWindow.h"

#include <QtCore/QDir>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractNetworkCache>
#include <QtWidgets/QMessageBox>

namespace Otter
{

TransfersManager* TransfersManager::m_instance(nullptr);
QVector<Transfer*> TransfersManager::m_transfers;
QVector<Transfer*> TransfersManager::m_privateTransfers;
bool TransfersManager::m_isInitilized(false);
bool TransfersManager::m_hasRunningTransfers(false);

Transfer::Transfer(TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(nullptr),
	m_device(nullptr),
	m_speed(0),
	m_bytesStart(0),
	m_bytesReceivedDifference(0),
	m_bytesReceived(0),
	m_bytesTotal(0),
	m_options(options),
	m_state(UnknownState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_remainingTime(-1),
	m_isSelectingPath(false),
	m_isArchived(false)
{
}

Transfer::Transfer(const QSettings &settings, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(nullptr),
	m_device(nullptr),
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
	m_state((m_bytesReceived > 0 && m_bytesTotal == m_bytesReceived && QFile::exists(settings.value(QLatin1String("target")).toString())) ? FinishedState : ErrorState),
	m_updateTimer(0),
	m_updateInterval(0),
	m_remainingTime(-1),
	m_isSelectingPath(false),
	m_isArchived(true)
{
	m_timeStarted.setTimeSpec(Qt::UTC);
	m_timeFinished.setTimeSpec(Qt::UTC);
}

Transfer::Transfer(const QUrl &source, const QString &target, TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(nullptr),
	m_device(nullptr),
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
	m_remainingTime(-1),
	m_isSelectingPath(false),
	m_isArchived(false)
{
	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setUrl(QUrl(source));

	start(NetworkManagerFactory::getNetworkManager(m_options.testFlag(IsPrivateOption))->get(request), target);
}

Transfer::Transfer(const QNetworkRequest &request, const QString &target, TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(nullptr),
	m_device(nullptr),
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
	m_remainingTime(-1),
	m_isSelectingPath(false),
	m_isArchived(false)
{
	start(NetworkManagerFactory::getNetworkManager(m_options.testFlag(IsPrivateOption))->get(request), target);
}

Transfer::Transfer(QNetworkReply *reply, const QString &target, TransferOptions options, QObject *parent) : QObject(parent ? parent : TransfersManager::getInstance()),
	m_reply(reply),
	m_source((m_reply->url().isValid() ? m_reply->url() : m_reply->request().url()).adjusted(QUrl::RemovePassword | QUrl::PreferLocalFile)),
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
	m_remainingTime(-1),
	m_isSelectingPath(false),
	m_isArchived(false)
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
		const qint64 oldSpeed(m_speed);

		m_speed = (m_bytesReceivedDifference * 2);
		m_bytesReceivedDifference = 0;

		if (m_speed != oldSpeed)
		{
			m_speeds.enqueue(m_speed);

			if (m_speeds.count() > 10)
			{
				m_speeds.dequeue();
			}

			if (m_bytesTotal > 0)
			{
				qint64 speedSum(0);

				for (int i = 0; i < m_speeds.count(); ++i)
				{
					speedSum += m_speeds.at(i);
				}

				speedSum /= m_speeds.count();

				m_remainingTime = qRound(static_cast<qreal>(m_bytesTotal - m_bytesReceived) / static_cast<qreal>(speedSum));
			}

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

	const QMimeDatabase mimeDatabase;

	m_reply = reply;
	m_mimeType = mimeDatabase.mimeTypeForName(m_reply->header(QNetworkRequest::ContentTypeHeader).toString());

	QString temporaryFileName(getSuggestedFileName());

	if (temporaryFileName.isEmpty())
	{
		temporaryFileName = QLatin1String("otter-download-XXXXXX.") + m_mimeType.preferredSuffix();
	}
	else if (temporaryFileName.contains(QLatin1Char('.')))
	{
		const QString suffix(mimeDatabase.suffixForFileName(temporaryFileName.simplified().remove(QLatin1Char(' '))));
		int position(temporaryFileName.lastIndexOf(QLatin1Char('.')));

		if (!suffix.isEmpty() && temporaryFileName.endsWith(suffix, Qt::CaseInsensitive))
		{
			position = (temporaryFileName.length() - suffix.length() - 1);
		}

		temporaryFileName = temporaryFileName.insert(position, QLatin1String("-XXXXXX"));
	}

	m_device = new QTemporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + temporaryFileName, this);
	m_timeStarted = QDateTime::currentDateTimeUtc();
	m_bytesTotal = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();

	if (m_bytesTotal == 0 && reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool() && reply->manager()->cache())
	{
		QIODevice *device(reply->manager()->cache()->data(m_source));

		if (device)
		{
			m_bytesTotal = device->size();

			device->close();
			device->deleteLater();
		}
	}

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

	handleDataAvailable();

	const bool isRunning(m_state == RunningState);

	if (isRunning)
	{
		connect(m_reply, &QNetworkReply::downloadProgress, this, &Transfer::handleDownloadProgress);
		connect(m_reply, &QNetworkReply::finished, this, &Transfer::handleDownloadFinished);
		connect(m_reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Transfer::handleDownloadError);
	}
	else
	{
		markAsFinished();

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}
	}

	m_device->reset();

	m_mimeType = mimeDatabase.mimeTypeForData(m_device);

	m_device->seek(m_device->size());

	handleDataAvailable();

	if (isRunning)
	{
		connect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
	}

	QString finalTarget;
	bool canOverwriteExisting(false);

	if (target.isEmpty())
	{
		if (!SettingsManager::getOption(SettingsManager::Browser_AlwaysAskWhereToSaveDownloadOption).toBool())
		{
			m_options |= IsQuickTransferOption;
		}

		const QString directory(m_options.testFlag(IsQuickTransferOption) ? Utils::normalizePath(SettingsManager::getOption(SettingsManager::Paths_DownloadsOption).toString()) : QString());
		const QString fileName(getSuggestedFileName());

		if (m_options.testFlag(IsQuickTransferOption))
		{
			finalTarget = directory + QDir::separator() + fileName;

			if (QFile::exists(finalTarget))
			{
				m_options |= CanAskForPathOption;
			}
		}

		if (m_options.testFlag(CanAskForPathOption))
		{
			m_isSelectingPath = true;

			const SaveInformation information(Utils::getSavePath(fileName, directory));

			m_isSelectingPath = false;

			if (!information.canSave)
			{
				if (m_reply)
				{
					m_reply->abort();
				}

				m_device = nullptr;

				cancel();

				return;
			}

			finalTarget = information.path;
			canOverwriteExisting = true;
		}

		finalTarget = QDir::toNativeSeparators(finalTarget);
	}
	else
	{
		finalTarget = QFileInfo(QDir::toNativeSeparators(target)).absoluteFilePath();
	}

	if (!finalTarget.isEmpty())
	{
		setTarget(finalTarget, canOverwriteExisting);
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
			m_mimeType = mimeDatabase.mimeTypeForFile(m_target);
		}
	}
}

void Transfer::openTarget() const
{
	Utils::runApplication(m_openCommand, QUrl::fromLocalFile(getTarget()));
}

void Transfer::cancel()
{
	m_state = CancelledState;

	if (m_reply)
	{
		m_reply->abort();

		QTimer::singleShot(250, m_reply, &QNetworkReply::deleteLater);
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

		QTimer::singleShot(250, m_reply, &QNetworkReply::deleteLater);
	}

	if (m_device && !m_device->inherits("QTemporaryFile"))
	{
		m_device->close();
		m_device->deleteLater();
		m_device = nullptr;
	}

	if (m_state == RunningState)
	{
		m_state = ErrorState;

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}
	}

	m_speeds.clear();

	emit stopped();
	emit changed();
}

void Transfer::markAsStarted()
{
	m_timeStarted = QDateTime::currentDateTimeUtc();
}

void Transfer::markAsFinished()
{
	m_timeFinished = QDateTime::currentDateTimeUtc();
}

void Transfer::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_bytesReceivedDifference += (bytesReceived - (m_bytesReceived - m_bytesStart));
	m_bytesReceived = (m_bytesStart + bytesReceived);
	m_bytesTotal = (m_bytesStart + bytesTotal);

	emit progressChanged(bytesReceived, bytesTotal);
}

void Transfer::handleDataAvailable()
{
	if (!m_reply || !m_device)
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

	if (m_state == RunningState && m_reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool() && m_bytesTotal >= 0 && m_device->size() == m_bytesTotal)
	{
		handleDownloadFinished();
	}
}

void Transfer::handleDownloadFinished()
{
	if (!m_reply)
	{
		if (m_device && !m_device->inherits("QTemporaryFile"))
		{
			m_device->close();
			m_device->deleteLater();
			m_device = nullptr;
		}

		if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
		{
			deleteLater();
		}

		return;
	}

	if (!m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isNull())
	{
		const QUrl url(m_source.resolved(m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl()));

		if (!url.isValid() || (m_source.scheme() == QLatin1String("https") && url.scheme() == QLatin1String("http")))
		{
			handleDownloadError(QNetworkReply::UnknownContentError);
		}
		else
		{
			m_source = url;

			restart();
		}

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

	disconnect(m_reply, &QNetworkReply::downloadProgress, this, &Transfer::handleDownloadProgress);
	disconnect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
	disconnect(m_reply, &QNetworkReply::finished, this, &Transfer::handleDownloadFinished);

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
		markAsFinished();

		m_state = FinishedState;
		m_mimeType = QMimeDatabase().mimeTypeForFile(m_target);
	}

	emit finished();
	emit changed();

	if (m_device && (m_options.testFlag(HasToOpenAfterFinishOption) || !m_device->inherits("QTemporaryFile")))
	{
		m_device->close();
		m_device->deleteLater();
		m_device = nullptr;

		if (m_reply)
		{
			QTimer::singleShot(250, m_reply, &QNetworkReply::deleteLater);
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

void Transfer::handleDownloadError(QNetworkReply::NetworkError error)
{
	Q_UNUSED(error)

	stop();

	m_state = ErrorState;

	if (m_options.testFlag(CanAutoDeleteOption) && !m_isSelectingPath)
	{
		deleteLater();
	}
}

void Transfer::setOpenCommand(const QString &command)
{
	m_openCommand = command;

	m_options |= HasToOpenAfterFinishOption;

	QTemporaryFile *file(qobject_cast<QTemporaryFile*>(m_device));

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

			m_device = nullptr;
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
		return (m_suggestedFileName.isEmpty() ? tr("file") : m_suggestedFileName);
	}

	QString fileName;

	if (m_reply->hasRawHeader(QStringLiteral("Content-Disposition").toLatin1()))
	{
		const QString contenDispositionHeader(m_reply->rawHeader(QStringLiteral("Content-Disposition").toLatin1()));

		if (contenDispositionHeader.contains(QLatin1String("filename*=")))
		{
			fileName = QRegularExpression(QLatin1String("[\\s;]filename\\*=[\"]?[a-zA-Z0-9\\-_]+\\'[a-zA-Z0-9\\-]?\\'([^\"]+)[\"]?[\\s;]?")).match(contenDispositionHeader).captured(1);
		}
		else
		{
			fileName = QRegularExpression(QLatin1String("[\\s;]filename=[\"]?([^\"]+)[\"]?[\\s;]?")).match(contenDispositionHeader).captured(1);
		}

		if (fileName.contains(QLatin1String("; ")))
		{
			fileName = fileName.section(QLatin1String("; "), 0, 0);
		}

		fileName = QUrl(fileName).fileName();
	}

	if (fileName.isEmpty())
	{
		fileName = m_source.fileName();
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

int Transfer::getRemainingTime() const
{
	return m_remainingTime;
}

bool Transfer::isArchived() const
{
	return m_isArchived;
}

bool Transfer::resume()
{
	if (m_state != ErrorState || !QFile::exists(m_target))
	{
		return false;
	}

	m_isArchived = false;

	if (m_bytesTotal == 0)
	{
		return restart();
	}

	QFile *file(new QFile(m_target));

	if (!file->open(QIODevice::WriteOnly | QIODevice::Append))
	{
		file->deleteLater();

		return false;
	}

	m_state = RunningState;
	m_device = file;
	m_timeStarted = QDateTime::currentDateTimeUtc();
	m_timeFinished = {};
	m_bytesStart = file->size();

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setRawHeader(QStringLiteral("Range").toLatin1(), QStringLiteral("bytes=%1-").arg(file->size()).toLatin1());
	request.setUrl(m_source);

	m_reply = NetworkManagerFactory::getNetworkManager(m_options.testFlag(IsPrivateOption))->get(request);

	handleDataAvailable();

	connect(m_reply, &QNetworkReply::downloadProgress, this, &Transfer::handleDownloadProgress);
	connect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
	connect(m_reply, &QNetworkReply::finished, this, &Transfer::handleDownloadFinished);
	connect(m_reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Transfer::handleDownloadError);

	if (m_updateTimer == 0 && m_updateInterval > 0)
	{
		m_updateTimer = startTimer(m_updateInterval);
	}

	return true;
}

bool Transfer::restart()
{
	stop();

	m_isArchived = false;

	QFile *file(new QFile(m_target));

	if (!file->open(QIODevice::WriteOnly))
	{
		file->deleteLater();

		return false;
	}

	m_state = RunningState;
	m_device = file;
	m_timeStarted = QDateTime::currentDateTimeUtc();
	m_timeFinished = {};
	m_bytesStart = 0;

	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setUrl(m_source);

	m_reply = NetworkManagerFactory::getNetworkManager(m_options.testFlag(IsPrivateOption))->get(request);

	handleDataAvailable();

	connect(m_reply, &QNetworkReply::downloadProgress, this, &Transfer::handleDownloadProgress);
	connect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
	connect(m_reply, &QNetworkReply::finished, this, &Transfer::handleDownloadFinished);
	connect(m_reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Transfer::handleDownloadError);

	if (m_updateTimer == 0 && m_updateInterval > 0)
	{
		m_updateTimer = startTimer(m_updateInterval);
	}

	return true;
}

bool Transfer::setTarget(const QString &target, bool canOverwriteExisting)
{
	if (m_target == target)
	{
		return false;
	}

	QString mutableTarget(target);

	if (!canOverwriteExisting && !m_options.testFlag(CanOverwriteOption) && QFile::exists(target))
	{
		const int result(QMessageBox::question(Application::getActiveWindow(), tr("Question"), tr("File with the same name already exists.\nDo you want to overwrite it?\n\n%1").arg(target), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel));

		if (result == QMessageBox::No)
		{
			const QFileInfo information(target);
			const QString path(Utils::getSavePath(information.fileName(), information.path(), {}, true).path);

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

		const bool success(QFile::rename(m_target, mutableTarget));

		if (success)
		{
			m_target = mutableTarget;
		}

		return success;
	}

	QFile *file(new QFile(mutableTarget, this));

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
			disconnect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
		}

		m_device->reset();

		file->write(m_device->readAll());

		m_device->close();
		m_device->deleteLater();
	}

	m_device = file;

	handleDataAvailable();

	if (!m_reply || m_reply->isFinished())
	{
		handleDownloadFinished();
	}
	else
	{
		connect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
	}

	return false;
}

TransfersManager::TransfersManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void TransfersManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new TransfersManager(QCoreApplication::instance());
	}
}

void TransfersManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		save();
	}
}

void TransfersManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void TransfersManager::updateRunningTransfersState()
{
	bool hasRunningTransfers(false);

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->getState() == Transfer::RunningState)
		{
			hasRunningTransfers = true;

			break;
		}
	}

	m_hasRunningTransfers = hasRunningTransfers;
}

void TransfersManager::addTransfer(Transfer *transfer)
{
	m_transfers.append(transfer);

	transfer->setUpdateInterval(500);

	connect(transfer, &Transfer::started, m_instance, &TransfersManager::handleTransferStarted);
	connect(transfer, &Transfer::finished, m_instance, &TransfersManager::handleTransferFinished);
	connect(transfer, &Transfer::changed, m_instance, &TransfersManager::handleTransferChanged);
	connect(transfer, &Transfer::stopped, m_instance, &TransfersManager::handleTransferStopped);

	if (transfer->getOptions().testFlag(Transfer::CanNotifyOption) && transfer->getState() != Transfer::CancelledState)
	{
		emit m_instance->transferStarted(transfer);

		if (transfer->getState() == Transfer::RunningState)
		{
			emit m_instance->transferFinished(transfer);
		}
	}

	if (transfer->getOptions().testFlag(Transfer::IsPrivateOption))
	{
		m_privateTransfers.append(transfer);
	}
}

void TransfersManager::save()
{
	if (SessionsManager::isReadOnly() || SettingsManager::getOption(SettingsManager::Browser_PrivateModeOption).toBool() || !SettingsManager::getOption(SettingsManager::History_RememberDownloadsOption).toBool())
	{
		return;
	}

	QSettings history(SessionsManager::getWritableDataPath(QLatin1String("transfers.ini")), QSettings::IniFormat);
	history.clear();

	const int limit(SettingsManager::getOption(SettingsManager::History_DownloadsLimitPeriodOption).toInt());
	int entry(1);

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_privateTransfers.contains(m_transfers.at(i)) || (m_transfers.at(i)->getState() == Transfer::FinishedState && m_transfers.at(i)->getTimeFinished().isValid() && m_transfers.at(i)->getTimeFinished().daysTo(QDateTime::currentDateTimeUtc()) > limit))
		{
			continue;
		}

		history.setValue(QStringLiteral("%1/source").arg(entry), m_transfers.at(i)->getSource().toString());
		history.setValue(QStringLiteral("%1/target").arg(entry), m_transfers.at(i)->getTarget());
		history.setValue(QStringLiteral("%1/timeStarted").arg(entry), m_transfers.at(i)->getTimeStarted().toString(Qt::ISODate));
		history.setValue(QStringLiteral("%1/timeFinished").arg(entry), ((m_transfers.at(i)->getTimeFinished().isValid() && m_transfers.at(i)->getState() != Transfer::RunningState) ? m_transfers.at(i)->getTimeFinished() : QDateTime::currentDateTimeUtc()).toString(Qt::ISODate));
		history.setValue(QStringLiteral("%1/bytesTotal").arg(entry), m_transfers.at(i)->getBytesTotal());
		history.setValue(QStringLiteral("%1/bytesReceived").arg(entry), m_transfers.at(i)->getBytesReceived());

		++entry;
	}

	history.sync();
}

void TransfersManager::clearTransfers(int period)
{
	for (int i = (m_transfers.count() - 1); i >= 0; --i)
	{
		if (m_transfers.at(i)->getState() == Transfer::FinishedState && (period == 0 || (m_transfers.at(i)->getTimeFinished().isValid() && m_transfers.at(i)->getTimeFinished().secsTo(QDateTime::currentDateTimeUtc()) > (period * 3600))))
		{
			TransfersManager::removeTransfer(m_transfers.at(i));
		}
	}
}

void TransfersManager::handleTransferStarted()
{
	Transfer *transfer(qobject_cast<Transfer*>(sender()));

	if (transfer && transfer->getState() != Transfer::CancelledState)
	{
		if (transfer->getState() == Transfer::RunningState)
		{
			m_hasRunningTransfers = true;
		}

		emit transferStarted(transfer);

		scheduleSave();
	}
}

void TransfersManager::handleTransferFinished()
{
	Transfer *transfer(qobject_cast<Transfer*>(sender()));

	updateRunningTransfersState();

	if (transfer)
	{
		if (transfer->getState() == Transfer::FinishedState)
		{
			connect(NotificationsManager::createNotification(NotificationsManager::TransferCompletedEvent, tr("Download completed:\n%1").arg(QFileInfo(transfer->getTarget()).fileName()), Notification::InformationLevel, this), &Notification::clicked, transfer, &Transfer::openTarget);
		}

		emit transferFinished(transfer);

		if (!m_privateTransfers.contains(transfer))
		{
			scheduleSave();
		}
	}
}

void TransfersManager::handleTransferChanged()
{
	Transfer *transfer(qobject_cast<Transfer*>(sender()));

	if (transfer)
	{
		scheduleSave();
		updateRunningTransfersState();

		emit transferChanged(transfer);
	}
}

void TransfersManager::handleTransferStopped()
{
	Transfer *transfer(qobject_cast<Transfer*>(sender()));

	updateRunningTransfersState();

	if (transfer)
	{
		emit transferStopped(transfer);

		scheduleSave();
	}
}

TransfersManager* TransfersManager::getInstance()
{
	return m_instance;
}

Transfer* TransfersManager::startTransfer(const QUrl &source, const QString &target, Transfer::TransferOptions options)
{
	Transfer *transfer(new Transfer(source, target, options, m_instance));

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return nullptr;
	}

	addTransfer(transfer);

	return transfer;
}

Transfer* TransfersManager::startTransfer(const QNetworkRequest &request, const QString &target, Transfer::TransferOptions options)
{
	Transfer *transfer(new Transfer(request, target, options, m_instance));

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return nullptr;
	}

	addTransfer(transfer);

	return transfer;
}

Transfer* TransfersManager::startTransfer(QNetworkReply *reply, const QString &target, Transfer::TransferOptions options)
{
	Transfer *transfer(new Transfer(reply, target, options, m_instance));

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return nullptr;
	}

	addTransfer(transfer);

	return transfer;
}

QVector<Transfer*> TransfersManager::getTransfers()
{
	if (!m_isInitilized)
	{
		QSettings history(SessionsManager::getWritableDataPath(QLatin1String("transfers.ini")), QSettings::IniFormat);
		const QStringList entries(history.childGroups());

		m_transfers.reserve(entries.count());

		for (int i = 0; i < entries.count(); ++i)
		{
			history.beginGroup(entries.at(i));

			if (!history.value(QLatin1String("source")).toString().isEmpty() && !history.value(QLatin1String("target")).toString().isEmpty())
			{
				addTransfer(new Transfer(history, m_instance));
			}

			history.endGroup();
		}

		m_isInitilized = true;

		connect(QCoreApplication::instance(), &Application::aboutToQuit, m_instance, &TransfersManager::save);
	}

	return m_transfers;
}

bool TransfersManager::removeTransfer(Transfer *transfer, bool keepFile)
{
	if (!transfer || !m_transfers.contains(transfer))
	{
		return false;
	}

	m_transfers.removeAll(transfer);

	m_privateTransfers.removeAll(transfer);

	if (transfer->getState() == Transfer::RunningState)
	{
		transfer->stop();
	}

	if (!keepFile && !transfer->getTarget().isEmpty() && QFile::exists(transfer->getTarget()))
	{
		QFile::remove(transfer->getTarget());
	}

	emit m_instance->transferRemoved(transfer);

	transfer->deleteLater();

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
		if (m_transfers.at(i)->getState() != Transfer::RunningState)
		{
			continue;
		}

		if (source.isEmpty() && m_transfers.at(i)->getTarget() == target)
		{
			return true;
		}

		if (target.isEmpty() && m_transfers.at(i)->getSource() == source)
		{
			return true;
		}

		if (!source.isEmpty() && !target.isEmpty() && m_transfers.at(i)->getSource() == source && m_transfers.at(i)->getTarget() == target)
		{
			return true;
		}
	}

	return false;
}

bool TransfersManager::hasRunningTransfers()
{
	return m_hasRunningTransfers;
}

}
