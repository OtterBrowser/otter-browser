/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebEngineTransfer.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>

namespace Otter
{

QtWebEngineTransfer::QtWebEngineTransfer(QWebEngineDownloadItem *item, TransferOptions options, QObject *parent) : Transfer(options, parent),
	m_item(item),
	m_suggestedFileName(QFileInfo(item->path()).fileName())
{
	m_item->accept();

	markStarted();

	connect(m_item, SIGNAL(finished()), this, SLOT(markFinished()));
	connect(m_item, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
}

void QtWebEngineTransfer::cancel()
{
	m_item->cancel();
}

QUrl QtWebEngineTransfer::getSource() const
{
	return m_item->url();
}

QString QtWebEngineTransfer::getSuggestedFileName()
{
	return m_suggestedFileName;
}

QString QtWebEngineTransfer::getTarget() const
{
	return m_item->path();
}

QMimeType QtWebEngineTransfer::getMimeType() const
{
	return QMimeDatabase().mimeTypeForName(m_item->mimeType());
}

qint64 QtWebEngineTransfer::getBytesReceived() const
{
	return m_item->receivedBytes();
}

qint64 QtWebEngineTransfer::getBytesTotal() const
{
	return m_item->totalBytes();
}

Transfer::TransferState QtWebEngineTransfer::getState() const
{
	switch (m_item->state())
	{
		case QWebEngineDownloadItem::DownloadRequested:
		case QWebEngineDownloadItem::DownloadInProgress:
			return RunningState;
		case QWebEngineDownloadItem::DownloadCompleted:
			return FinishedState;
		case QWebEngineDownloadItem::DownloadCancelled:
			return CancelledState;
		case QWebEngineDownloadItem::DownloadInterrupted:
			return ErrorState;
		default:
			return UnknownState;
	}

	return UnknownState;
}

bool QtWebEngineTransfer::setTarget(const QString &target)
{
	m_item->setPath(target);
}

}
