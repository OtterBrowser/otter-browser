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

#include "Console.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

Console* Console::m_instance(nullptr);
QVector<Console::Message> Console::m_messages;

Console::Console(QObject *parent) : QObject(parent)
{
}

void Console::createInstance()
{
	if (!m_instance)
	{
		m_instance = new Console(QCoreApplication::instance());
	}
}

void Console::addMessage(const QString &note, MessageCategory category, MessageLevel level, const QString &source, int line, quint64 window)
{
	Message message;
	message.note = note;
	message.source = source;
	message.category = category;
	message.level = level;
	message.line = line;
	message.window = window;

	m_messages.append(message);

	if (m_messages.count() > 1000)
	{
		m_messages.removeFirst();
	}

	emit m_instance->messageAdded(message);
}

Console* Console::getInstance()
{
	return m_instance;
}

QVector<Console::Message> Console::getMessages()
{
	return m_messages;
}

}
