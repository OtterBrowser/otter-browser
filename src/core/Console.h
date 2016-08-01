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

#ifndef OTTER_CONSOLE_H
#define OTTER_CONSOLE_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>

namespace Otter
{

class Console : public QObject
{
	Q_OBJECT

public:
	enum MessageCategory
	{
		OtherCategory = 0,
		NetworkCategory = 1,
		SecurityCategory = 2,
		JavaScriptCategory = 3
	};

	enum MessageLevel
	{
		UnknownLevel = 0,
		DebugLevel = 1,
		LogLevel = 2,
		WarningLevel = 3,
		ErrorLevel = 4
	};

	struct Message
	{
		QDateTime time;
		QString note;
		QString source;
		MessageCategory category;
		MessageLevel level;
		quint64 window;
		int line;

		Message() : category(OtherCategory), level(UnknownLevel), window(0), line(-1) {}
	};

	static void createInstance(QObject *parent = NULL);
	static void addMessage(const QString &note, MessageCategory category, MessageLevel level, const QString &source = QString(), int line = -1, quint64 window = 0);
	static Console* getInstance();
	static QList<Message> getMessages();

protected:
	explicit Console(QObject *parent = NULL);

private:
	static Console *m_instance;
	static QList<Message> m_messages;

signals:
	void messageAdded(const Console::Message &message);
};

}

#endif
