/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "InputInterpreter.h"
#include "BookmarksManager.h"
#include "SearchEnginesManager.h"
#include "Utils.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

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

		emit requestedSearch(m_text, SettingsManager::getOption(SettingsManager::Search_DefaultSearchEngineOption).toString(), m_hints);

		m_text.clear();
	}
}

void InputInterpreter::verifyLookup(const QHostInfo &host)
{
	killTimer(m_timer);

	m_timer = 0;

	if (host.error() == QHostInfo::NoError)
	{
		emit requestedOpenUrl(QUrl::fromUserInput(m_text), m_hints);
	}
	else
	{
		emit requestedSearch(m_text, QString(), m_hints);
	}

	deleteLater();
}

void InputInterpreter::interpret(const QString &text, WindowsManager::OpenHints hints, bool ignoreBookmarks)
{
	if (text.startsWith(QLatin1String("bookmarks:")))
	{
		BookmarksItem *bookmark(text.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getItem(text.mid(11)) : BookmarksManager::getBookmark(text.mid(10).toULongLong()));

		if (bookmark)
		{
			emit requestedOpenBookmark(bookmark, hints);

			deleteLater();

			return;
		}
	}

	if (!ignoreBookmarks)
	{
		BookmarksItem *bookmark(BookmarksManager::getBookmark(text));

		if (bookmark)
		{
			emit requestedOpenBookmark(bookmark, hints);

			deleteLater();

			return;
		}
	}

	const QString localPath(Utils::normalizePath(text));

	if (localPath != text)
	{
		emit requestedOpenUrl(localPath, hints);

		deleteLater();

		return;
	}

	const QFileInfo fileInformation(text);

	if (fileInformation.exists() && fileInformation.isAbsolute())
	{
		emit requestedOpenUrl(QUrl::fromLocalFile(QFileInfo(text).canonicalFilePath()), hints);

		deleteLater();

		return;
	}

	const QUrl url(QUrl::fromUserInput(text));
	const QHostAddress address(text);

	if (!address.isNull() || (url.isValid() && (url.isLocalFile() || QRegularExpression(QLatin1String("^(\\w+\\:\\S+)|([\\w\\-]+\\.[a-zA-Z]{2,}(/\\S*)?$)")).match(text).hasMatch())))
	{
		emit requestedOpenUrl(url, hints);

		deleteLater();

		return;
	}

	const QString keyword(text.section(QLatin1Char(' '), 0, 0));
	const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(keyword, true));

	if (!searchEngine.identifier.isEmpty())
	{
		emit requestedSearch(text.section(QLatin1Char(' '), 1), searchEngine.identifier, hints);

		deleteLater();

		return;
	}

	if (keyword == QLatin1String("?"))
	{
		emit requestedSearch(text.section(QLatin1Char(' '), 1), QString(), hints);

		deleteLater();

		return;
	}

	const int lookupTimeout(SettingsManager::getOption(SettingsManager::AddressField_HostLookupTimeoutOption).toInt());

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

