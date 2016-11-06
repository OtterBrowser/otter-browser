/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QTWEBENGINEWEBBACKEND_H
#define OTTER_QTWEBENGINEWEBBACKEND_H

#include "../../../../core/WebBackend.h"

#include <QtWebEngineWidgets/QWebEngineDownloadItem>

namespace Otter
{

class QtWebEnginePage;
class QtWebEngineUrlRequestInterceptor;

class QtWebEngineWebBackend : public WebBackend
{
	Q_OBJECT

public:
	explicit QtWebEngineWebBackend(QObject *parent = nullptr);

	WebWidget* createWidget(bool isPrivate = false, ContentsWidget *parent = nullptr);
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getEngineVersion() const;
	QString getSslVersion() const;
	QString getUserAgent(const QString &pattern = QString()) const;
	QStringList getBlockedElements(const QString &domain) const;
	QUrl getHomePage() const;
	QIcon getIcon() const;
	bool requestThumbnail(const QUrl &url, const QSize &size);

protected slots:
	void optionChanged(int identifier);
	void downloadFile(QWebEngineDownloadItem *item);

private:
	QtWebEngineUrlRequestInterceptor *m_requestInterceptor;
	bool m_isInitialized;

	static QString m_engineVersion;
	static QMap<QString, QString> m_userAgentComponents;
	static QMap<QString, QString> m_userAgents;

friend class QtWebEnginePage;
};

}

#endif
