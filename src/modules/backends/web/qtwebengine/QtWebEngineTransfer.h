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

#ifndef OTTER_QTWEBENGINETRANSFER_H
#define OTTER_QTWEBENGINETRANSFER_H

#include "../../../../core/TransfersManager.h"

#include <QtWebEngineWidgets/QWebEngineDownloadItem>

namespace Otter
{

class QtWebEngineTransfer : public Transfer
{
	Q_OBJECT

public:
	explicit QtWebEngineTransfer(QWebEngineDownloadItem *item, TransferOptions options = CanAskForPathOption, QObject *parent = NULL);

	QUrl getSource() const;
	QString getSuggestedFileName();
	QString getTarget() const;
	QMimeType getMimeType() const;
	qint64 getBytesReceived() const;
	qint64 getBytesTotal() const;
	TransferState getState() const;

public slots:
	void cancel();
	bool setTarget(const QString &target);

private:
	QPointer<QWebEngineDownloadItem> m_item;
	QString m_suggestedFileName;
};

}

#endif
