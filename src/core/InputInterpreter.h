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

#ifndef OTTER_INPUTINTERPRETER_H
#define OTTER_INPUTINTERPRETER_H

#include "BookmarksModel.h"

namespace Otter
{

class InputInterpreter final : public QObject
{
	Q_OBJECT

public:
	enum InterpreterFlag
	{
		NoFlags = 0,
		NoBookmarkKeywordsFlag = 1,
		NoHostLookupFlag = 2,
		NoSearchKeywordsFlag = 4
	};

	Q_DECLARE_FLAGS(InterpreterFlags, InterpreterFlag)

	struct InterpreterResult final
	{
		enum ResultType
		{
			UnknownType = 0,
			BookmarkType,
			SearchType,
			UrlType
		};

		BookmarksModel::Bookmark *bookmark = nullptr;
		QString searchEngine;
		QString searchQuery;
		QUrl url;
		ResultType type = UnknownType;

		bool isValid() const
		{
			return (type != UnknownType);
		}
	};

	explicit InputInterpreter(QObject *parent = nullptr);

	static InterpreterResult interpret(const QString &text, InterpreterFlags flags = NoFlags);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::InputInterpreter::InterpreterFlags)

#endif
