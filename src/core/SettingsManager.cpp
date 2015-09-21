/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "SettingsManager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>

namespace Otter
{

SettingsManager* SettingsManager::m_instance = NULL;
QString SettingsManager::m_globalPath;
QString SettingsManager::m_overridePath;
QHash<QString, QVariant> SettingsManager::m_defaults;

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
}

void SettingsManager::createInstance(const QString &path, QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SettingsManager(parent);
		m_globalPath = path + QLatin1String("/otter.conf");
		m_overridePath = path + QLatin1String("/override.ini");

		QSettings defaults(QLatin1String(":/schemas/options.ini"), QSettings::IniFormat);
		const QStringList groups = defaults.childGroups();

		for (int i = 0; i < groups.count(); ++i)
		{
			defaults.beginGroup(groups.at(i));

			const QStringList keys = defaults.childGroups();

			for (int j = 0; j < keys.count(); ++j)
			{
				m_defaults[QStringLiteral("%1/%2").arg(groups.at(i)).arg(keys.at(j))] = defaults.value(QStringLiteral("%1/value").arg(keys.at(j)));
			}

			defaults.endGroup();
		}

		m_defaults[QLatin1String("Paths/Downloads")] = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		m_defaults[QLatin1String("Paths/SaveFile")] = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
	}
}

void SettingsManager::removeOverride(const QUrl &url, const QString &key)
{
	if (key.isEmpty())
	{
		QSettings(m_overridePath, QSettings::IniFormat).remove(url.isLocalFile() ? QLatin1String("localhost") : url.host());
	}
	else
	{
		QSettings(m_overridePath, QSettings::IniFormat).remove((url.isLocalFile() ? QLatin1String("localhost") : url.host()) + QLatin1Char('/') + key);
	}
}

void SettingsManager::setValue(const QString &key, const QVariant &value, const QUrl &url)
{
	if (!url.isEmpty())
	{
		if (value.isNull())
		{
			QSettings(m_overridePath, QSettings::IniFormat).remove((url.isLocalFile() ? QLatin1String("localhost") : url.host()) + QLatin1Char('/') + key);
		}
		else
		{
			QSettings(m_overridePath, QSettings::IniFormat).setValue((url.isLocalFile() ? QLatin1String("localhost") : url.host()) + QLatin1Char('/') + key, value);
		}

		return;
	}

	if (getValue(key) != value)
	{
		QSettings(m_globalPath, QSettings::IniFormat).setValue(key, value);

		emit m_instance->valueChanged(key, value);
	}
}

SettingsManager* SettingsManager::getInstance()
{
	return m_instance;
}

QString SettingsManager::getReport()
{
	QString report;
	QTextStream stream(&report);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Settings\n");

	QStringList excludeValues;
	QSettings defaults(QLatin1String(":/schemas/options.ini"), QSettings::IniFormat);
	const QStringList groups = defaults.childGroups();

	for (int i = 0; i < groups.count(); ++i)
	{
		defaults.beginGroup(groups.at(i));

		const QStringList keys = defaults.childGroups();

		for (int j = 0; j < keys.count(); ++j)
		{
			const QString type = defaults.value(QStringLiteral("%1/type").arg(keys.at(j))).toString();

			if (type == QLatin1String("string") || type == QLatin1String("path"))
			{
				excludeValues.append(QStringLiteral("%1/%2").arg(groups.at(i)).arg(keys.at(j)));
			}
		}

		defaults.endGroup();
	}

	QStringList keys = m_defaults.keys();
	keys.sort();

	for (int i = 0; i < keys.count(); ++i)
	{
		stream << QLatin1Char('\t');
		stream.setFieldWidth(50);
		stream << keys.at(i);
		stream.setFieldWidth(20);

		if (excludeValues.contains(keys.at(i)))
		{
			stream << QLatin1Char('-');
		}
		else
		{
			stream << m_defaults[keys.at(i)].toString();
		}

		stream << ((m_defaults[keys.at(i)] == getValue(keys.at(i))) ? QLatin1String("default") : QLatin1String("non default"));
		stream.setFieldWidth(0);
		stream << QLatin1Char('\n');
	}

	stream << QLatin1Char('\n');

	return report;
}

QVariant SettingsManager::getDefaultValue(const QString &key)
{
	return m_defaults[key];
}

QVariant SettingsManager::getValue(const QString &key, const QUrl &url)
{
	if (!url.isEmpty())
	{
		return QSettings(m_overridePath, QSettings::IniFormat).value((url.isLocalFile() ? QLatin1String("localhost") : url.host()) + QLatin1Char('/') + key, getValue(key));
	}

	return QSettings(m_globalPath, QSettings::IniFormat).value(key, getDefaultValue(key));
}

bool SettingsManager::hasOverride(const QUrl &url, const QString &key)
{
	if (key.isEmpty())
	{
		return QSettings(m_overridePath, QSettings::IniFormat).childGroups().contains(url.isLocalFile() ? QLatin1String("localhost") : url.host());
	}
	else
	{
		return QSettings(m_overridePath, QSettings::IniFormat).contains((url.isLocalFile() ? QLatin1String("localhost") : url.host()) + QLatin1Char('/') + key);
	}
}

}
