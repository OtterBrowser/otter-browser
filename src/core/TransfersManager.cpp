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

#include "TransfersManager.h"
#include "SessionsManager.h"
#include "Transfer.h"
#include "../ui/MainWindow.h"

#include <QtCore/QSettings>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

TransfersManager* TransfersManager::m_instance = NULL;
QVector<Transfer*> TransfersManager::m_transfers;
bool TransfersManager::m_initilized = false;

TransfersManager::TransfersManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void TransfersManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new TransfersManager(parent);
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

void TransfersManager::addTransfer(Transfer *transfer)
{
	m_transfers.prepend(transfer);

	connect(transfer, SIGNAL(started()), m_instance, SLOT(transferStarted()));
	connect(transfer, SIGNAL(finished()), m_instance, SLOT(transferFinished()));
	connect(transfer, SIGNAL(changed()), m_instance, SLOT(transferChanged()));
	connect(transfer, SIGNAL(stopped()), m_instance, SLOT(transferStopped()));

	emit m_instance->transferStarted(transfer);

	if (transfer->getState() != Transfer::RunningState)
	{
		emit m_instance->transferFinished(transfer);
	}
}

void TransfersManager::save()
{
	if (SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool() || !SettingsManager::getValue(QLatin1String("History/RememberDownloads")).toBool())
	{
		return;
	}

	QSettings history(SessionsManager::getProfilePath() + QLatin1String("/transfers.ini"), QSettings::IniFormat);
	history.clear();

	const int limit = SettingsManager::getValue(QLatin1String("History/DownloadsLimitPeriod")).toInt();
	int entry = 1;

	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->isPrivate() || (m_transfers.at(i)->getState() == Transfer::FinishedState && m_transfers.at(i)->getTimeFinished().isValid() && m_transfers.at(i)->getTimeFinished().daysTo(QDateTime::currentDateTime()) > limit))
		{
			continue;
		}

		history.setValue(QStringLiteral("%1/source").arg(entry), m_transfers.at(i)->getSource().toString());
		history.setValue(QStringLiteral("%1/target").arg(entry), m_transfers.at(i)->getTarget());
		history.setValue(QStringLiteral("%1/timeStarted").arg(entry), m_transfers.at(i)->getTimeStarted().toString(Qt::ISODate));
		history.setValue(QStringLiteral("%1/timeFinished").arg(entry), ((m_transfers.at(i)->getTimeFinished().isValid() && m_transfers.at(i)->getState() != Transfer::RunningState) ? m_transfers.at(i)->getTimeFinished() : QDateTime::currentDateTime()).toString(Qt::ISODate));
		history.setValue(QStringLiteral("%1/bytesTotal").arg(entry), m_transfers.at(i)->getBytesTotal());
		history.setValue(QStringLiteral("%1/bytesReceived").arg(entry), m_transfers.at(i)->getBytesReceived());

		++entry;
	}

	history.sync();
}

void TransfersManager::transferStarted()
{
	Transfer *transfer = qobject_cast<Transfer*>(sender());

	if (transfer)
	{
		emit transferStarted(transfer);

		scheduleSave();
	}
}

void TransfersManager::transferFinished()
{
	Transfer *transfer = qobject_cast<Transfer*>(sender());

	if (transfer)
	{
		emit transferFinished(transfer);

		if (transfer->isPrivate())
		{
			removeTransfer(transfer);
		}
		else
		{
			scheduleSave();
		}
	}
}

void TransfersManager::transferChanged()
{
	Transfer *transfer = qobject_cast<Transfer*>(sender());

	if (transfer)
	{
		emit transferChanged(transfer);

		scheduleSave();
	}
}

void TransfersManager::transferStopped()
{
	Transfer *transfer = qobject_cast<Transfer*>(sender());

	if (transfer)
	{
		emit transferStopped(transfer);

		scheduleSave();
	}
}

void TransfersManager::clearTransfers(int period)
{
	for (int i = 0; i < m_transfers.count(); ++i)
	{
		if (m_transfers.at(i)->getState() == Transfer::FinishedState && (period == 0 || (m_transfers.at(i)->getTimeFinished().isValid() && m_transfers.at(i)->getTimeFinished().secsTo(QDateTime::currentDateTime()) > (period * 3600))))
		{
			TransfersManager::removeTransfer(m_transfers.at(i));
		}
	}
}

TransfersManager* TransfersManager::getInstance()
{
	return m_instance;
}

Transfer* TransfersManager::startTransfer(const QUrl &source, const QString &target, bool quickTransfer, bool isPrivate)
{
	Transfer *transfer = new Transfer(source, target, quickTransfer, isPrivate, m_instance);

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return NULL;
	}

	addTransfer(transfer);

	return transfer;
}

Transfer* TransfersManager::startTransfer(const QNetworkRequest &request, const QString &target, bool quickTransfer, bool isPrivate)
{
	Transfer *transfer = new Transfer(request, target, quickTransfer, isPrivate, m_instance);

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return NULL;
	}

	addTransfer(transfer);

	return transfer;
}

Transfer* TransfersManager::startTransfer(QNetworkReply *reply, const QString &target, bool quickTransfer, bool isPrivate)
{
	Transfer *transfer = new Transfer(reply, target, quickTransfer, isPrivate, m_instance);

	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return NULL;
	}

	addTransfer(transfer);

	return transfer;
}

QString TransfersManager::getSavePath(const QString &fileName, QString path)
{
	do
	{
		if (path.isEmpty())
		{
			QFileDialog dialog(SessionsManager::getActiveWindow(), tr("Save File"), SettingsManager::getValue(QLatin1String("Paths/SaveFile")).toString() + QDir::separator() + fileName, tr("All files (*)"));
			dialog.setFileMode(QFileDialog::AnyFile);
			dialog.setAcceptMode(QFileDialog::AcceptSave);

			if (dialog.exec() == QDialog::Rejected || dialog.selectedFiles().isEmpty())
			{
				break;
			}

			path = dialog.selectedFiles().first();
		}

		const bool exists = QFile::exists(path);

		if (isDownloading(QString(), path))
		{
			path = QString();

			if (QMessageBox::warning(SessionsManager::getActiveWindow(), tr("Warning"), tr("Target path is already used by another transfer.\nSelect another one."), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Cancel)
			{
				break;
			}
		}
		else if ((exists && !QFileInfo(path).isWritable()) || (!exists && !QFileInfo(QFileInfo(path).dir().path()).isWritable()))
		{
			path = QString();

			if (QMessageBox::warning(SessionsManager::getActiveWindow(), tr("Warning"), tr("Target path is not writable.\nSelect another one."), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Cancel)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	while (true);

	if (!path.isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Paths/SaveFile"), QFileInfo(path).dir().canonicalPath());
	}

	return path;
}

QVector<Transfer*> TransfersManager::getTransfers()
{
	if (!m_initilized)
	{
		m_initilized = true;

		QSettings history(SessionsManager::getProfilePath() + QLatin1String("/transfers.ini"), QSettings::IniFormat);
		const QStringList entries = history.childGroups();

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

		connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), m_instance, SLOT(save()));
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

	transfer->stop();

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

}
