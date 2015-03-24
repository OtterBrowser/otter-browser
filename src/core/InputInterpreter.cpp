/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "InputInterpreter.h"
#include "BookmarksManager.h"
#include "SearchesManager.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>

namespace Otter
{

InputInterpreter::InputInterpreter(QObject *parent) : QObject(parent),
	m_lookup(0),
	m_timer(0)
{
}

void InputInterpreter::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_timer)
	{
		QHostInfo::abortHostLookup(m_lookup);

		killTimer(m_timer);

		m_timer = 0;

		emit requestedSearch(m_text, SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString(), m_hints);

		m_text.clear();
	}
}

void InputInterpreter::verifyLookup(const QHostInfo &host)
{
	killTimer(m_timer);

	m_timer = 0;

	if (host.error() == QHostInfo::NoError)
	{
		emit requestedOpenUrl(QUrl(m_text), m_hints);
	}
	else
	{
		emit requestedSearch(m_text, QString(), m_hints);
	}

	deleteLater();
}

void InputInterpreter::interpret(const QString &text, OpenHints hints, bool ignoreBookmarks)
{
	if (!ignoreBookmarks)
	{
		BookmarksItem *bookmark = BookmarksManager::getBookmark(text);

		if (bookmark)
		{
			emit requestedOpenBookmark(bookmark, hints);

			deleteLater();

			return;
		}
	}

	if (text == QString(QLatin1Char('~')) || text.startsWith(QLatin1Char('~') + QDir::separator()))
	{
		const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

		if (!locations.isEmpty())
		{
			emit requestedOpenUrl(locations.first() + text.mid(1), hints);

			deleteLater();

			return;
		}
	}

	if (QFileInfo(text).exists())
	{
		emit requestedOpenUrl(QUrl::fromLocalFile(QFileInfo(text).canonicalFilePath()), hints);

		deleteLater();

		return;
	}

	const QUrl url = QUrl::fromUserInput(text);
	const QHostAddress address(text);

	if (!address.isNull() || (url.isValid() && (url.isLocalFile() || QRegularExpression(QLatin1String("^(\\w+\\:\\S+)|([\\w\\-]+\\.[a-zA-Z]{2,}(/\\S*)?$)")).match(text).hasMatch())))
	{
		emit requestedOpenUrl(url, hints);

		deleteLater();

		return;
	}

	const QString keyword = text.section(QLatin1Char(' '), 0, 0);
	const SearchInformation engine = SearchesManager::getSearchEngine(keyword, true);

	if (!engine.identifier.isEmpty())
	{
		emit requestedSearch(text.section(QLatin1Char(' '), 1), engine.identifier, hints);

		deleteLater();

		return;
	}

	if (keyword == QLatin1String("?"))
	{
		emit requestedSearch(text.section(QLatin1Char(' '), 1), QString(), hints);

		deleteLater();

		return;
	}

	const int lookupTimeout = SettingsManager::getValue(QLatin1String("AddressField/HostLookupTimeout")).toInt();

	if (url.isValid() && lookupTimeout > 0)
	{
		m_text = text;
		m_hints = hints;

		if (m_timer != 0)
		{
			QHostInfo::abortHostLookup(m_lookup);

			killTimer(m_timer);

			m_timer = 0;
		}

		m_lookup = QHostInfo::lookupHost(url.host(), this, SLOT(verifyLookup(QHostInfo)));
		m_timer = startTimer(lookupTimeout);

		return;
	}

	emit requestedSearch(text, QString(), hints);

	deleteLater();

	return;
}

}

