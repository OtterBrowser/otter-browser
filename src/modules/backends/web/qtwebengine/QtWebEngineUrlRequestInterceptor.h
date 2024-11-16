/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QTWEBENGINEURLREQUESTINTERCEPTOR_H
#define OTTER_QTWEBENGINEURLREQUESTINTERCEPTOR_H

#include "QtWebEngineWebWidget.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"

#include <QtWebEngineCore/QWebEngineUrlRequestInterceptor>

namespace Otter
{

class QtWebEngineUrlRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
	Q_OBJECT

public:
	explicit QtWebEngineUrlRequestInterceptor(QtWebEngineWebWidget *parent);

	void interceptRequest(QWebEngineUrlRequestInfo &request) override;
	QStringList getBlockedElements() const;
	QVector<NetworkManager::ResourceInformation> getBlockedRequests() const;

protected:
	void updateOptions(const QUrl &url);
	QVariant getOption(int identifier, const QUrl &url) const;
	QVariant getPageInformation(WebWidget::PageInformation key) const;

protected slots:
	void resetStatistics();

private:
	QtWebEngineWebWidget *m_widget;
	QString m_acceptLanguage;
	QString m_userAgent;
	QStringList m_blockedElements;
	QStringList m_unblockedHosts;
	QVector<NetworkManager::ResourceInformation> m_blockedRequests;
	QVector<int> m_contentBlockingProfiles;
	NetworkManagerFactory::DoNotTrackPolicy m_doNotTrackPolicy;
	quint64 m_startedRequestsAmount;
	bool m_areImagesEnabled;
	bool m_canSendReferrer;
	bool m_isWorkingOffline;

	static WebBackend *m_backend;

signals:
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void requestBlocked(const NetworkManager::ResourceInformation &request);

friend class QtWebEngineWebWidget;
};

}

#endif
