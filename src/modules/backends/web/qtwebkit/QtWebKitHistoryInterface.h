/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBKITHISTORYINTERFACE_H
#define OTTER_QTWEBKITHISTORYINTERFACE_H

#include <QtCore/QStringList>
#include <QtWebKit/QWebHistoryInterface>

namespace Otter
{

class QtWebKitHistoryInterface final : public QWebHistoryInterface
{
	Q_OBJECT

public:
	explicit QtWebKitHistoryInterface(QObject *parent);

	void addHistoryEntry(const QString &url) override;
	bool historyContains(const QString &url) const override;

protected slots:
	void clear();

private:
	QStringList m_urls;
};

}

#endif
