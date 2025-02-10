/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "IniSettings.h"

#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QTextStream>

namespace Otter
{

IniSettings::IniSettings(QObject *parent) : QObject(parent),
	m_hasError(false)
{
}

IniSettings::IniSettings(const QString &path, QObject *parent) : QObject(parent),
	m_path(path),
	m_hasError(false)
{
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		m_hasError = true;

		return;
	}

	QString group;
	QStringList comment;
	bool isHeader(true);
	QTextStream stream(&file);

	while (!stream.atEnd())
	{
		const QString line(stream.readLine());

		if (line.startsWith(QLatin1Char(';')))
		{
			if (isHeader)
			{
				comment.append(line.mid(2));
			}
		}
		else if (line.startsWith(QLatin1Char('[')))
		{
			if (line.endsWith(QLatin1Char(']')))
			{
				group = line.mid(1, (line.length() - 2));
			}
		}
		else if (!line.isEmpty())
		{
			const QString key(line.section(QLatin1Char('='), 0, 0));
			const QVariant value(line.contains(QLatin1Char('=')) ? QVariant(line.section(QLatin1Char('='), 1, -1)) : QVariant());

			if (!key.isEmpty())
			{
				if (!m_data.contains(group))
				{
					m_data[group] = {};
				}

				m_data[group][key] = value;
			}
		}
		else if (isHeader)
		{
			isHeader = false;
		}
	}

	m_comment = comment.join(QLatin1Char('\n'));

	file.close();
}

void IniSettings::clear()
{
	m_data.clear();
}

void IniSettings::beginGroup(const QString &group)
{
	m_group = group;
}

void IniSettings::removeGroup(const QString &group)
{
	if (m_data.contains(group))
	{
		m_data.remove(group);
	}

	if (m_group == group)
	{
		m_group.clear();
	}
}

void IniSettings::endGroup()
{
	m_group.clear();
}

void IniSettings::setComment(const QString &comment)
{
	m_comment = comment;
}

void IniSettings::setValue(const QString &key, const QVariant &value)
{
	if (m_group.isEmpty())
	{
		return;
	}

	if (!value.isNull())
	{
		if (!m_data.contains(m_group))
		{
			m_data[m_group] = {};
		}

		m_data[m_group][key] = value;
	}
	else if (m_data.contains(m_group) && m_data[m_group].contains(key))
	{
		m_data[m_group].remove(key);
	}
}

QString IniSettings::getComment() const
{
	return m_comment;
}

QVariant IniSettings::getValue(const QString &key, const QVariant &fallback) const
{
	if (m_data.contains(m_group) && m_data[m_group].contains(key))
	{
		return m_data[m_group][key];
	}

	return fallback;
}

QStringList IniSettings::getGroups() const
{
	return m_data.keys();
}

QStringList IniSettings::getKeys() const
{
	if (!m_group.isEmpty())
	{
		if (m_data.contains(m_group))
		{
			return m_data[m_group].keys();
		}

		return {};
	}

	QStringList keys;
	keys.reserve(m_data.count());

	QMap<QString, QVariantMap>::const_iterator iterator;

	for (iterator = m_data.constBegin(); iterator != m_data.constEnd(); ++iterator)
	{
		keys.append(iterator.value().keys());
	}

	return keys;
}

bool IniSettings::save(const QString &path, bool isAtomic)
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

	QTextStream stream(file);
	bool canAddNewLine(false);

	if (!m_comment.isEmpty())
	{
		const QStringList comment(m_comment.split(QLatin1Char('\n')));

		for (int i = 0; i < comment.count(); ++i)
		{
			stream << QLatin1String("; ") << comment.at(i) << QLatin1Char('\n');
		}

		canAddNewLine = true;
	}

	QMap<QString, QVariantMap>::iterator groupsIterator;

	for (groupsIterator = m_data.begin(); groupsIterator != m_data.end(); ++groupsIterator)
	{
		if (groupsIterator.value().isEmpty())
		{
			continue;
		}

		if (canAddNewLine)
		{
			stream << QLatin1Char('\n');
		}
		else
		{
			canAddNewLine = true;
		}

		stream << QLatin1Char('[') << groupsIterator.key() << QLatin1String("]\n");

		QVariantMap::iterator keysIterator;

		for (keysIterator = groupsIterator.value().begin(); keysIterator != groupsIterator.value().end(); ++keysIterator)
		{
			stream << keysIterator.key() << QLatin1Char('=') << keysIterator.value().toString() << QLatin1Char('\n');
		}
	}

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

bool IniSettings::hasError() const
{
	return m_hasError;
}

}
