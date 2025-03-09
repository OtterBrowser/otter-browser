/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "JsonSettings.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QTextStream>

namespace Otter
{

JsonSettings::JsonSettings() :
	m_hasError(false)
{
}

JsonSettings::JsonSettings(const QString &path) :
	m_path(path),
	m_hasError(false)
{
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	if (file.read(2) == QByteArray(2, '/'))
	{
		file.reset();

		QStringList comment;
		QTextStream stream(&file);

		while (!stream.atEnd())
		{
			const QString line(stream.readLine());

			if (!line.startsWith(QLatin1String("//")))
			{
				break;
			}

			comment.append(line.mid(3));
		}

		m_comment = comment.join(QLatin1Char('\n'));

		file.seek(stream.pos());
	}
	else
	{
		file.reset();
	}

	const QJsonDocument document(QJsonDocument::fromJson(file.readAll()));

	if (document.isArray())
	{
		setArray(document.array());
	}
	else
	{
		setObject(document.object());
	}

	file.close();
}

void JsonSettings::setComment(const QString &comment)
{
	m_comment = comment;
}

QRect JsonSettings::readRectangle(const QVariant &value)
{
	QRect rectangle;

	switch (value.type())
	{
		case QVariant::String:
			{
				const QStringList list(value.toString().split(QLatin1Char(',')));

				if (list.count() == 4)
				{
					rectangle = {list.at(0).simplified().toInt(), list.at(1).simplified().toInt(), list.at(2).simplified().toInt(), list.at(3).simplified().toInt()};
				}
			}

			break;
		case QVariant::Map:
			{
				const QVariantMap map(value.toMap());

				rectangle = {map.value(QLatin1String("x")).toInt(), map.value(QLatin1String("y")).toInt(), map.value(QLatin1String("width")).toInt(), map.value(QLatin1String("height")).toInt()};
			}

			break;
		case QVariant::Rect:
			rectangle = value.toRect();

			break;
		default:
			break;
	}

	return rectangle;
}

QPoint JsonSettings::readPoint(const QVariant &value)
{
	QPoint point;

	switch (value.type())
	{
		case QVariant::String:
			{
				const QStringList list(value.toString().split(QLatin1Char(',')));

				if (list.count() == 2)
				{
					point = {list.at(0).simplified().toInt(), list.at(1).simplified().toInt()};
				}
			}

			break;
		case QVariant::Map:
			{
				const QVariantMap map(value.toMap());

				point = {map.value(QLatin1String("x")).toInt(), map.value(QLatin1String("y")).toInt()};
			}

			break;
		case QVariant::Point:
			point = value.toPoint();

			break;
		default:
			break;
	}

	return point;
}

QSize JsonSettings::readSize(const QVariant &value)
{
	QSize size;

	switch (value.type())
	{
		case QVariant::String:
			{
				const QStringList list(value.toString().split(QLatin1Char(',')));

				if (list.count() == 2)
				{
					size = {list.at(0).simplified().toInt(), list.at(1).simplified().toInt()};
				}
			}

			break;
		case QVariant::Map:
			{
				const QVariantMap map(value.toMap());

				size = {map.value(QLatin1String("width")).toInt(), map.value(QLatin1String("height")).toInt()};
			}

			break;
		case QVariant::Size:
			size = value.toSize();

			break;
		default:
			break;
	}

	return size;
}

QString JsonSettings::getComment() const
{
	return m_comment;
}

bool JsonSettings::save(const QString &path, bool isAtomic)
{
	if (path.isEmpty() && m_path.isEmpty())
	{
		m_hasError = true;

		return false;
	}

	QFileDevice *file(nullptr);

	if (isAtomic)
	{
		file = new QSaveFile(path.isEmpty() ? m_path : path);
	}
	else
	{
		file = new QFile(path.isEmpty() ? m_path : path);
	}

	if (!file->open(QIODevice::WriteOnly))
	{
		m_hasError = true;

		file->deleteLater();

		return false;
	}

	m_hasError = false;

	if (!m_comment.isEmpty())
	{
		QTextStream stream(file);
		const QStringList comment(m_comment.split(QLatin1Char('\n')));

		for (int i = 0; i < comment.count(); ++i)
		{
			stream << QLatin1String("// ") << comment.at(i) << QLatin1Char('\n');
		}

		stream << QLatin1Char('\n');
	}

	QByteArray data(toJson());
	int spacesAmount(0);
	int tabsAmount(0);
	int lineStartPosition(0);
	bool isReplacingSpaces(true);

	for (int i = 0; i < data.length(); ++i)
	{
		const char character(data.at(i));

		if (character == '\n')
		{
			spacesAmount = 0;
			tabsAmount = 0;
			lineStartPosition = (i + 1);
			isReplacingSpaces = true;
		}
		else if (isReplacingSpaces)
		{
			if (character == ' ')
			{
				++spacesAmount;

				if (spacesAmount == 4)
				{
					spacesAmount = 0;

					++tabsAmount;
				}
			}
			else
			{
				if (tabsAmount > 0)
				{
					data.replace(lineStartPosition, (tabsAmount * 4), QByteArray(tabsAmount, '\t'));

					i -= (tabsAmount * 3);
				}

				isReplacingSpaces = false;
			}
		}
	}

	file->write(data);

	bool result(true);

	if (isAtomic)
	{
		result = qobject_cast<QSaveFile*>(file)->commit();
	}
	else
	{
		file->close();
	}

	file->deleteLater();

	return result;
}

bool JsonSettings::hasError() const
{
	return m_hasError;
}

}
