/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtWebEngineCore/QWebEngineUrlRequestInterceptor>

namespace Otter
{

class QtWebEngineUrlRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
	Q_OBJECT

public:
	explicit QtWebEngineUrlRequestInterceptor(QObject *parent = nullptr);

	void interceptRequest(QWebEngineUrlRequestInfo &request) override;
	QStringList getBlockedElements(const QString &domain) const;

protected:
	void timerEvent(QTimerEvent *event) override;
	void clearContentBlockingInformation();

protected slots:
	void handleOptionChanged(int identifier);

private:
	QMap<QString, QStringList> m_blockedElements;
	QMap<QString, QVector<int> > m_contentBlockingProfiles;
	int m_clearTimer;
	bool m_areImagesEnabled;
};

}

#endif
