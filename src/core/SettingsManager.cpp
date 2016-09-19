/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014, 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>

namespace Otter
{

SettingsManager* SettingsManager::m_instance = NULL;
QString SettingsManager::m_globalPath;
QString SettingsManager::m_overridePath;
QHash<QString, int> SettingsManager::m_customOptions;
QHash<int, SettingsManager::OptionDefinition> SettingsManager::m_definitions;
int SettingsManager::m_identifierCounter = -1;
int SettingsManager::m_optionIdentifierEnumerator = 0;

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
}

void SettingsManager::createInstance(const QString &path, QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SettingsManager(parent);
		m_optionIdentifierEnumerator = m_instance->metaObject()->indexOfEnumerator(QLatin1String("OptionIdentifier").data());
		m_globalPath = path + QLatin1String("/otter.conf");
		m_overridePath = path + QLatin1String("/override.ini");

		QSettings defaults(QLatin1String(":/schemas/options.ini"), QSettings::IniFormat);
		const QStringList groups(defaults.childGroups());

		for (int i = 0; i < groups.count(); ++i)
		{
			defaults.beginGroup(groups.at(i));

			const QStringList keys(defaults.childGroups());

			for (int j = 0; j < keys.count(); ++j)
			{
				defaults.beginGroup(keys.at(j));

				const QString type(defaults.value(QLatin1String("type")).toString());
				OptionDefinition definition;
				definition.defaultValue = defaults.value(QLatin1String("value"));
				definition.flags = (IsEnabledFlag | IsVisibleFlag | IsBuiltInFlag);
				definition.identifier = getOptionIdentifier(QStringLiteral("%1/%2").arg(groups.at(i)).arg(keys.at(j)));

				if (type == QLatin1String("bool"))
				{
					definition.type = BooleanType;
				}
				else if (type == QLatin1String("color"))
				{
					definition.type = ColorType;
				}
				else if (type == QLatin1String("enumeration"))
				{
					definition.type = EnumerationType;
					definition.choices = defaults.value(QLatin1String("choices")).toStringList();
				}
				else if (type == QLatin1String("font"))
				{
					definition.type = FontType;
				}
				else if (type == QLatin1String("icon"))
				{
					definition.type = IconType;
				}
				else if (type == QLatin1String("integer"))
				{
					definition.type = IntegerType;
				}
				else if (type == QLatin1String("list"))
				{
					definition.type = ListType;
				}
				else if (type == QLatin1String("path"))
				{
					definition.type = PathType;
				}
				else if (type == QLatin1String("string"))
				{
					definition.type = StringType;
				}
				else
				{
					definition.type = UnknownType;
				}

				m_definitions[definition.identifier] = definition;

				defaults.endGroup();
			}

			defaults.endGroup();
		}

		m_definitions[Paths_DownloadsOption].defaultValue = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		m_definitions[Paths_SaveFileOption].defaultValue = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
	}

	m_identifierCounter = m_instance->metaObject()->enumerator(m_optionIdentifierEnumerator).keyCount();
}

void SettingsManager::removeOverride(const QUrl &url, const QString &key)
{
	if (key.isEmpty())
	{
		QSettings(m_overridePath, QSettings::IniFormat).remove(getHost(url));
	}
	else
	{
		QSettings(m_overridePath, QSettings::IniFormat).remove(getHost(url) + QLatin1Char('/') + key);
	}
}

void SettingsManager::updateOptionDefinition(int identifier, const SettingsManager::OptionDefinition &definition)
{
	if (m_definitions.contains(identifier))
	{
		m_definitions[identifier].defaultValue = definition.defaultValue;
		m_definitions[identifier].choices = definition.choices;
	}
}

void SettingsManager::setValue(int identifier, const QVariant &value, const QUrl &url)
{
	const QString name(getOptionName(identifier));

	if (!url.isEmpty())
	{
		if (value.isNull())
		{
			QSettings(m_overridePath, QSettings::IniFormat).remove(getHost(url) + QLatin1Char('/') + name);
		}
		else
		{
			QSettings(m_overridePath, QSettings::IniFormat).setValue(getHost(url) + QLatin1Char('/') + name, value);
		}

		emit m_instance->valueChanged(identifier, value, url);

		return;
	}

	if (getValue(identifier) != value)
	{
		QSettings(m_globalPath, QSettings::IniFormat).setValue(name, value);

		emit m_instance->valueChanged(identifier, value);
	}
}

SettingsManager* SettingsManager::getInstance()
{
	return m_instance;
}

QString SettingsManager::getOptionName(int identifier)
{
	QString name(m_instance->metaObject()->enumerator(m_optionIdentifierEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(6);

		return name.replace(QLatin1Char('_'), QLatin1Char('/'));
	}

	return m_customOptions.key(identifier);
}

QString SettingsManager::getHost(const QUrl &url)
{
	return (url.isLocalFile() ? QLatin1String("localhost") : url.host());
}

QString SettingsManager::getReport()
{
	QString report;
	QTextStream stream(&report);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Settings:\n");

	QStringList excludeValues;
	QSettings defaults(QLatin1String(":/schemas/options.ini"), QSettings::IniFormat);
	const QStringList defaultsGroups(defaults.childGroups());

	for (int i = 0; i < defaultsGroups.count(); ++i)
	{
		defaults.beginGroup(defaultsGroups.at(i));

		const QStringList keys(defaults.childGroups());

		for (int j = 0; j < keys.count(); ++j)
		{
			const QString type(defaults.value(QStringLiteral("%1/type").arg(keys.at(j))).toString());

			if (type == QLatin1String("string") || type == QLatin1String("path"))
			{
				excludeValues.append(QStringLiteral("%1/%2").arg(defaultsGroups.at(i)).arg(keys.at(j)));
			}
		}

		defaults.endGroup();
	}

	QHash<QString, int> overridenValues;
	QSettings overrides(m_overridePath, QSettings::IniFormat);
	const QStringList overridesGroups(overrides.childGroups());

	for (int i = 0; i < overridesGroups.count(); ++i)
	{
		overrides.beginGroup(overridesGroups.at(i));

		const QStringList keys(overrides.allKeys());

		for (int j = 0; j < keys.count(); ++j)
		{
			if (overridenValues.contains(keys.at(j)))
			{
				++overridenValues[keys.at(j)];
			}
			else
			{
				overridenValues[keys.at(j)] = 1;
			}
		}

		overrides.endGroup();
	}

	const QList<int> options(m_definitions.keys());
	QStringList optionNames;

	for (int i = 0; i < options.count(); ++i)
	{
		optionNames.append(getOptionName(options.at(i)));
	}

	optionNames.sort();

	for (int i = 0; i < optionNames.count(); ++i)
	{
		const int identifier(getOptionIdentifier(optionNames.at(i)));
		const QVariant defaultValue(m_definitions.contains(identifier) ? m_definitions[identifier].defaultValue : QVariant());

		stream << QLatin1Char('\t');
		stream.setFieldWidth(50);
		stream << optionNames.at(i);
		stream.setFieldWidth(20);

		if (excludeValues.contains(optionNames.at(i)))
		{
			stream << QLatin1Char('-');
		}
		else
		{
			stream << defaultValue.toString();
		}

		stream << ((defaultValue == getValue(identifier)) ? QLatin1String("default") : QLatin1String("non default"));
		stream << (overridenValues.contains(optionNames.at(i)) ? QStringLiteral("%1 override(s)").arg(overridenValues[optionNames.at(i)]) : QLatin1String("no overrides"));
		stream.setFieldWidth(0);
		stream << QLatin1Char('\n');
	}

	stream << QLatin1Char('\n');

	return report;
}

QVariant SettingsManager::getValue(int identifier, const QUrl &url)
{
	if (!m_definitions.contains(identifier))
	{
		return QVariant();
	}

	const QString name(getOptionName(identifier));

	if (!url.isEmpty())
	{
		return QSettings(m_overridePath, QSettings::IniFormat).value(getHost(url) + QLatin1Char('/') + name, getValue(identifier));
	}

	return QSettings(m_globalPath, QSettings::IniFormat).value(name, m_definitions[identifier].defaultValue);
}

QStringList SettingsManager::getOptions()
{
	const QList<int> options(m_definitions.keys());
	QStringList optionNames;

	for (int i = 0; i < options.count(); ++i)
	{
		optionNames.append(getOptionName(options.at(i)));
	}

	optionNames.sort();

	return optionNames;
}

SettingsManager::OptionDefinition SettingsManager::getOptionDefinition(int identifier)
{
	if (m_definitions.contains(identifier))
	{
		return m_definitions[identifier];
	}

	return OptionDefinition();
}

int SettingsManager::registerOption(const QString &name, const OptionDefinition &definition)
{
	if (name.isEmpty() || getOptionIdentifier(name) >= 0)
	{
		return -1;
	}

	const int identifier(m_identifierCounter);

	++m_identifierCounter;

	m_customOptions[name] = identifier;

	m_definitions[identifier] = definition;
	m_definitions[identifier].flags &= ~IsBuiltInFlag;

	return identifier;
}

int SettingsManager::getOptionIdentifier(const QString &name)
{
	QString mutableName(name);
	mutableName.replace(QLatin1Char('/'), QLatin1Char('_'));

	if (!name.endsWith(QLatin1String("Option")))
	{
		mutableName.append(QLatin1String("Option"));
	}

	if (m_customOptions.contains(name))
	{
		return m_customOptions[name];
	}

	return m_instance->metaObject()->enumerator(m_optionIdentifierEnumerator).keyToValue(mutableName.toLatin1());
}

bool SettingsManager::hasOverride(const QUrl &url, const QString &key)
{
	if (key.isEmpty())
	{
		return QSettings(m_overridePath, QSettings::IniFormat).childGroups().contains(getHost(url));
	}

	return QSettings(m_overridePath, QSettings::IniFormat).contains(getHost(url) + QLatin1Char('/') + key);
}

}
