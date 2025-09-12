/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBENGINETRANSFER_H
#define OTTER_QTWEBENGINETRANSFER_H

#include "../../../../core/TransfersManager.h"

#include <QtWebEngineWidgets/QWebEngineDownloadItem>

namespace Otter
{

class QtWebEngineTransfer final : public Transfer
{
	Q_OBJECT

public:
	explicit QtWebEngineTransfer(QWebEngineDownloadItem *item, TransferOptions options = CanAskForPathOption, QObject *parent = nullptr);

	QUrl getSource() const override;
	QString getSuggestedFileName() override;
	QString getTarget() const override;
	QMimeType getMimeType() const override;
	qint64 getBytesReceived() const override;
	qint64 getBytesTotal() const override;
	TransferState getState() const override;

public slots:
	void cancel() override;
	bool setTarget(const QString &target, bool canOverwriteExisting = false) override;

private:
	QPointer<QWebEngineDownloadItem> m_item;
};

}

#endif
