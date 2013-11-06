#include "SettingsManager.h"

#include <QtCore/QSettings>

namespace Otter
{

SettingsManager* SettingsManager::m_instance = NULL;
QHash<QString, QVariant> SettingsManager::m_defaultSettings;

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
}

void SettingsManager::createInstance(QObject *parent)
{
	m_instance = new SettingsManager(parent);
}

void SettingsManager::restore(const QString &key)
{
	QSettings().remove(key);
}

void SettingsManager::remove(const QString &key)
{
	QSettings().remove(key);
}

void SettingsManager::setDefaultValue(const QString &key, const QVariant &value)
{
	m_defaultSettings[key] = value;
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
	QSettings().setValue(key, value);
}

QVariant SettingsManager::keyValue(const QString &key)
{
	return m_defaultSettings[key];
}

QVariant SettingsManager::getDefaultValue(const QString &key)
{
	return m_instance->keyValue(key);
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &value)
{
	const QVariant defaultKeyValue = getDefaultValue(key);

	return QSettings().value(key, (defaultKeyValue.isNull() ? value : defaultKeyValue));
}

QStringList SettingsManager::valueKeys()
{
	return m_defaultSettings.keys();
}

QStringList SettingsManager::getKeys(const QString &pattern)
{
	if (pattern.isEmpty())
	{
		return m_instance->valueKeys();
	}

	const QStringList keys = m_instance->valueKeys();
	QStringList matched;

	for (int i = 0; keys.count(); ++i)
	{
		if (keys.at(i).contains(pattern))
		{
			matched.append(keys.at(i));
		}
	}

	return matched;
}

bool SettingsManager::contains(const QString &key)
{
	return m_instance->valueKeys().contains(key);
}

}
