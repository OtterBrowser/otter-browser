/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "SettingsManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtNetwork/QHostInfo>

namespace Otter
{

InputInterpreter::InputInterpreter(QObject *parent) : QObject(parent)
{
}

InputInterpreter::InterpreterResult InputInterpreter::interpret(const QString &text, InterpreterFlags flags)
{
	InterpreterResult result;

	if (text.isEmpty())
	{
		return result;
	}

	if (!flags.testFlag(NoSearchKeywordsFlag))
	{
		const QString keyword(text.section(QLatin1Char(' '), 0, 0));
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(keyword, true));

		if (searchEngine.isValid())
		{
			result.searchEngine = searchEngine.identifier;
			result.searchQuery = text.section(QLatin1Char(' '), 1);
			result.type = InterpreterResult::SearchType;

			return result;
		}

		if (keyword == QLatin1String("?"))
		{
			result.searchQuery = text.section(QLatin1Char(' '), 1);
			result.type = InterpreterResult::SearchType;

			return result;
		}
	}

	if (text.startsWith(QLatin1String("bookmarks:")))
	{
		BookmarksModel::Bookmark *bookmark(text.startsWith(QLatin1String("bookmarks:/")) ? BookmarksManager::getModel()->getBookmarkByPath(text.mid(11)) : BookmarksManager::getBookmark(text.mid(10).toULongLong()));

		if (bookmark)
		{
			result.bookmark = bookmark;
			result.type = InterpreterResult::BookmarkType;

			return result;
		}
	}

	if (!flags.testFlag(NoBookmarkKeywordsFlag))
	{
		BookmarksModel::Bookmark *bookmark(BookmarksManager::getBookmark(text));

		if (bookmark)
		{
			result.bookmark = bookmark;
			result.type = InterpreterResult::BookmarkType;

			return result;
		}
	}

	const QString localPath(Utils::normalizePath(text));

	if (localPath != text)
	{
		result.url = QUrl::fromLocalFile(localPath);
		result.type = InterpreterResult::UrlType;

		return result;
	}

	const QFileInfo fileInformation(text);

	if (fileInformation.exists() && fileInformation.isAbsolute())
	{
		result.url = QUrl::fromLocalFile(fileInformation.canonicalFilePath());
		result.type = InterpreterResult::UrlType;

		return result;
	}

	const QUrl url(QUrl::fromUserInput(text));

	if (!QHostAddress(text).isNull() || (url.isValid() && (url.isLocalFile() || QRegularExpression(QLatin1String(R"(^(\w+\:\S+)|([\w\-]+\.[a-zA-Z]{2,}(/\S*)?$))")).match(text).hasMatch())))
	{
		result.url = url;
		result.type = InterpreterResult::UrlType;

		return result;
	}

	if (!flags.testFlag(NoHostLookupFlag))
	{
		const int lookupTimeout(SettingsManager::getOption(SettingsManager::AddressField_HostLookupTimeoutOption).toInt());

		if (url.isValid() && lookupTimeout > 0)
		{
			QEventLoop eventLoop;
			const int lookupIdentifier(QHostInfo::lookupHost(url.host(), [&](const QHostInfo &information)
			{
				if (information.error() == QHostInfo::NoError)
				{
					result.url = QUrl::fromUserInput(text);
					result.type = InterpreterResult::UrlType;
				}

				eventLoop.quit();
			}));

			QTimer timer;
			timer.setSingleShot(true);

			connect(&timer, &QTimer::timeout, QCoreApplication::instance(), [&]()
			{
				QHostInfo::abortHostLookup(lookupIdentifier);

				eventLoop.quit();
			});

			timer.start(lookupTimeout);

			if (result.type == InterpreterResult::UnknownType)
			{
				eventLoop.exec();
			}

			if (result.type == InterpreterResult::UrlType)
			{
				return result;
			}
		}
	}

	result.searchQuery = text;
	result.type = InterpreterResult::SearchType;

	return result;
}

}
