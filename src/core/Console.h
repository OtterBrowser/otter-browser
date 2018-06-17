/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONSOLE_H
#define OTTER_CONSOLE_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QVector>

namespace Otter
{

class Console final : public QObject
{
	Q_OBJECT

public:
	enum MessageCategory
	{
		OtherCategory = 0,
		NetworkCategory,
		ContentFiltersCategory,
		SecurityCategory,
		CssCategory,
		JavaScriptCategory
	};

	enum MessageLevel
	{
		UnknownLevel = 0,
		DebugLevel,
		LogLevel,
		WarningLevel,
		ErrorLevel
	};

	struct Message final
	{
		QDateTime time = QDateTime::currentDateTimeUtc();
		QString note;
		QString source;
		MessageCategory category = OtherCategory;
		MessageLevel level = UnknownLevel;
		quint64 window = 0;
		int line = -1;
	};

	static void createInstance();
	static void addMessage(const QString &note, MessageCategory category, MessageLevel level, const QString &source = {}, int line = -1, quint64 window = 0);
	static Console* getInstance();
	static QVector<Console::Message> getMessages();

protected:
	explicit Console(QObject *parent = nullptr);

private:
	static Console *m_instance;
	static QVector<Message> m_messages;

signals:
	void messageAdded(const Console::Message &message);
};

}

#endif
