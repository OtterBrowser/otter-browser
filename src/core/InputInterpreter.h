/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_INPUTINTERPRETER_H
#define OTTER_INPUTINTERPRETER_H

#include "../core/WindowsManager.h"

#include <QtNetwork/QHostInfo>

namespace Otter
{

class InputInterpreter : public QObject
{
	Q_OBJECT

public:
	explicit InputInterpreter(QObject *parent = NULL);

	void interpret(const QString &text, WindowsManager::OpenHints hints, bool ignoreBookmarks = false);

protected:
	void timerEvent(QTimerEvent *event);

protected slots:
	void verifyLookup(const QHostInfo &host);

private:
	QString m_text;
	WindowsManager::OpenHints m_hints;
	int m_lookup;
	int m_timer;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark, WindowsManager::OpenHints hints);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

#endif
