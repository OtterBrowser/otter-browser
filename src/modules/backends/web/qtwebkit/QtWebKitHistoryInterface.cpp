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

#include "QtWebKitHistoryInterface.h"
#include "../../../../core/HistoryManager.h"

namespace Otter
{

QtWebKitHistoryInterface::QtWebKitHistoryInterface(QObject *parent) : QWebHistoryInterface(parent)
{
	connect(HistoryManager::getBrowsingHistoryModel(), &HistoryModel::cleared, this, &QtWebKitHistoryInterface::clear);
}

void QtWebKitHistoryInterface::clear()
{
	m_urls.clear();
}

void QtWebKitHistoryInterface::addHistoryEntry(const QString &url)
{
	if (m_urls.contains(url))
	{
		return;
	}

	m_urls.append(url);

	if (m_urls.length() > 100)
	{
		m_urls.removeAt(0);
	}
}

bool QtWebKitHistoryInterface::historyContains(const QString &url) const
{
	return (m_urls.contains(url) || HistoryManager::hasEntry(url));
}

}
