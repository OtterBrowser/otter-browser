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

#include "TransfersManager.h"
#include "Application.h"
#include "HistoryManager.h"
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
#include <QtCore/QTimeZone>
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
	m_timeStarted.setTimeZone(QTimeZone::utc());
	m_timeFinished.setTimeZone(QTimeZone::utc());
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
		const qint64 previousSpeed(m_speed);

		m_speed = (m_bytesReceivedDifference * 2);
		m_bytesReceivedDifference = 0;

		if (m_speed == previousSpeed)
		{
			return;
		}

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
	m_source = reply->request().url().adjusted(QUrl::RemovePassword | QUrl::PreferLocalFile);
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
		connect(m_reply, &QNetworkReply::errorOccurred, this, &Transfer::handleDownloadError);
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
			handleDownloadError(QNetworkReply::InsecureRedirectError);
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

void Transfer::setHash(QCryptographicHash::Algorithm algorithm, const QByteArray &hash)
{
	if (!hash.isEmpty())
	{
		m_hashes[algorithm] = hash;
	}
	else if (m_hashes.contains(algorithm))
	{
		m_hashes.remove(algorithm);
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

	if (m_reply->hasRawHeader(QByteArrayLiteral("Content-Disposition")))
	{
		const QString contenDispositionHeader(QString::fromLatin1(m_reply->rawHeader(QByteArrayLiteral("Content-Disposition"))));

		if (contenDispositionHeader.contains(QLatin1String("filename*=")))
		{
			fileName = QRegularExpression(QLatin1String(R"([\s;]filename\*=["]?[a-zA-Z0-9\-_]+\'[a-zA-Z0-9\-]?\'([^"]+)["]?[\s;]?)")).match(contenDispositionHeader).captured(1);
		}
		else
		{
			fileName = QRegularExpression(QLatin1String(R"([\s;]filename=["]?([^"]+)["]?[\s;]?)")).match(contenDispositionHeader).captured(1);
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

QIcon Transfer::getIcon() const
{
	return ThemesManager::getFileTypeIcon(getMimeType());
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

bool Transfer::verifyHashes() const
{
	if (m_state != FinishedState)
	{
		return false;
	}

	QFile file(getTarget());

	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QHash<QCryptographicHash::Algorithm, QByteArray>::const_iterator iterator;
	bool isValid(true);

	for (iterator = m_hashes.constBegin(); iterator != m_hashes.constEnd(); ++iterator)
	{
		QCryptographicHash hash(iterator.key());
		hash.addData(&file);

		if (hash.result() != iterator.value())
		{
			isValid = false;

			break;
		}

		file.seek(0);
	}

	file.close();

	return isValid;
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
	request.setRawHeader(QByteArrayLiteral("Range"), QStringLiteral("bytes=%1-").arg(file->size()).toLatin1());
	request.setUrl(m_source);

	m_reply = NetworkManagerFactory::getNetworkManager(m_options.testFlag(IsPrivateOption))->get(request);

	handleDataAvailable();

	connect(m_reply, &QNetworkReply::downloadProgress, this, &Transfer::handleDownloadProgress);
	connect(m_reply, &QNetworkReply::readyRead, this, &Transfer::handleDataAvailable);
	connect(m_reply, &QNetworkReply::finished, this, &Transfer::handleDownloadFinished);
	connect(m_reply, &QNetworkReply::errorOccurred, this, &Transfer::handleDownloadError);

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
	connect(m_reply, &QNetworkReply::errorOccurred, this, &Transfer::handleDownloadError);

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

		const bool isSuccess(QFile::rename(m_target, mutableTarget));

		if (isSuccess)
		{
			m_target = mutableTarget;
		}

		return isSuccess;
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
	if (Application::isAboutToQuit())
	{
		if (m_saveTimer != 0)
		{
			killTimer(m_saveTimer);

			m_saveTimer = 0;
		}

		save();
	}
	else if (m_saveTimer == 0)
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
	if (!transfer)
	{
		return;
	}

	const Transfer::TransferOptions options(transfer->getOptions());

	m_transfers.append(transfer);

	transfer->setUpdateInterval(500);

	connect(transfer, &Transfer::started, m_instance, [=]()
	{
		if (transfer->getState() != Transfer::CancelledState)
		{
			if (transfer->getState() == Transfer::RunningState)
			{
				m_hasRunningTransfers = true;
			}

			emit m_instance->transferStarted(transfer);
			emit m_instance->transfersChanged();

			m_instance->scheduleSave();
		}
	});
	connect(transfer, &Transfer::finished, m_instance, [=]()
	{
		m_instance->updateRunningTransfersState();

		if (transfer->getState() == Transfer::FinishedState)
		{
			Notification::Message message;
			message.message = QFileInfo(transfer->getTarget()).fileName();
			message.icon = transfer->getIcon();
			message.event = NotificationsManager::TransferCompletedEvent;

			if (message.icon.isNull())
			{
				message.icon = ThemesManager::createIcon(QLatin1String("download"));
			}

			connect(NotificationsManager::createNotification(message, m_instance), &Notification::clicked, transfer, &Transfer::openTarget);
		}

		emit m_instance->transferFinished(transfer);
		emit m_instance->transfersChanged();

		if (!m_privateTransfers.contains(transfer))
		{
			m_instance->scheduleSave();
		}
	});
	connect(transfer, &Transfer::changed, m_instance, [=]()
	{
		m_instance->scheduleSave();
		m_instance->updateRunningTransfersState();

		emit m_instance->transferChanged(transfer);
		emit m_instance->transfersChanged();
	});
	connect(transfer, &Transfer::stopped, m_instance, [=]()
	{
		m_instance->updateRunningTransfersState();

		emit m_instance->transferStopped(transfer);
		emit m_instance->transfersChanged();

		m_instance->scheduleSave();
	});

	if (options.testFlag(Transfer::CanNotifyOption) && transfer->getState() != Transfer::CancelledState)
	{
		emit m_instance->transferStarted(transfer);
		emit m_instance->transfersChanged();

		if (transfer->getState() == Transfer::RunningState)
		{
			emit m_instance->transferFinished(transfer);
		}
	}

	if (options.testFlag(Transfer::IsPrivateOption))
	{
		m_privateTransfers.append(transfer);
	}
	else
	{
		const QUrl source(transfer->getSource());
		const QString scheme(source.scheme());

		if (scheme == QLatin1String("http") || scheme == QLatin1String("https"))
		{
			HistoryManager::addEntry(source);
		}
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
		Transfer *transfer(m_transfers.at(i));

		if (m_privateTransfers.contains(transfer) || (transfer->getState() == Transfer::FinishedState && transfer->getTimeFinished().isValid() && transfer->getTimeFinished().daysTo(QDateTime::currentDateTimeUtc()) > limit))
		{
			continue;
		}

		history.setValue(QStringLiteral("%1/source").arg(entry), transfer->getSource().toString());
		history.setValue(QStringLiteral("%1/target").arg(entry), transfer->getTarget());
		history.setValue(QStringLiteral("%1/timeStarted").arg(entry), transfer->getTimeStarted().toString(Qt::ISODate));
		history.setValue(QStringLiteral("%1/timeFinished").arg(entry), ((transfer->getTimeFinished().isValid() && transfer->getState() != Transfer::RunningState) ? transfer->getTimeFinished() : QDateTime::currentDateTimeUtc()).toString(Qt::ISODate));
		history.setValue(QStringLiteral("%1/bytesTotal").arg(entry), transfer->getBytesTotal());
		history.setValue(QStringLiteral("%1/bytesReceived").arg(entry), transfer->getBytesReceived());

		++entry;
	}

	history.sync();
}

void TransfersManager::clearTransfers(int period)
{
	for (int i = (m_transfers.count() - 1); i >= 0; --i)
	{
		Transfer *transfer(m_transfers.at(i));

		if (transfer->getState() == Transfer::FinishedState && (period == 0 || (transfer->getTimeFinished().isValid() && transfer->getTimeFinished().secsTo(QDateTime::currentDateTimeUtc()) > (period * 3600))))
		{
			TransfersManager::removeTransfer(transfer);
		}
	}
}

TransfersManager* TransfersManager::getInstance()
{
	return m_instance;
}

Transfer* TransfersManager::startTransfer(const QUrl &source, const QString &target, Transfer::TransferOptions options)
{
	QNetworkRequest request;
	request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
#if QT_VERSION < 0x060000
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());
	request.setUrl(QUrl(source));

	Transfer *transfer(new Transfer(options, m_instance));
	transfer->start(NetworkManagerFactory::getNetworkManager(options.testFlag(Transfer::IsPrivateOption))->get(request), target);

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
	Transfer *transfer(new Transfer(options, m_instance));
	transfer->start(NetworkManagerFactory::getNetworkManager(options.testFlag(Transfer::IsPrivateOption))->get(request), target);

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
	Transfer *transfer(new Transfer(options, m_instance));
	transfer->start(reply, target);

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

TransfersManager::ActiveTransfersInformation TransfersManager::getActiveTransfersInformation()
{
	ActiveTransfersInformation information;

	if (!m_hasRunningTransfers)
	{
		return information;
	}

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		const Transfer *transfer(m_transfers.at(i));

		if (transfer->getState() != Transfer::RunningState)
		{
			continue;
		}

		if (transfer->getBytesTotal() > 0)
		{
			++information.activeTransfersAmount;

			information.bytesTotal += transfer->getBytesTotal();
			information.bytesReceived += transfer->getBytesReceived();
		}
		else
		{
			++information.unknownProgressTransfersAmount;
		}
	}

	return information;
}

int TransfersManager::getRunningTransfersCount()
{
	int runningTransfers(0);

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->getState() == Transfer::RunningState)
		{
			++runningTransfers;
		}
	}

	return runningTransfers;
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
	emit m_instance->transfersChanged();

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
		Transfer *transfer(m_transfers.at(i));

		if (transfer->getState() != Transfer::RunningState)
		{
			continue;
		}

		if (source.isEmpty() && transfer->getTarget() == target)
		{
			return true;
		}

		if (target.isEmpty() && transfer->getSource() == source)
		{
			return true;
		}

		if (!source.isEmpty() && !target.isEmpty() && transfer->getSource() == source && transfer->getTarget() == target)
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
