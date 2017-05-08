/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SpellCheckManager.h"
#include "SessionsManager.h"
#ifdef OTTER_ENABLE_SPELLCHECK
#include "../../3rdparty/sonnet/src/core/speller.h"
#endif

#include <QtCore/QLocale>

namespace Otter
{

SpellCheckManager* SpellCheckManager::m_instance(nullptr);
QString SpellCheckManager::m_defaultDictionary;
QMap<QString, QString> SpellCheckManager::m_dictionaries;

SpellCheckManager::SpellCheckManager(QObject *parent) : QObject(parent)
{
#ifdef OTTER_ENABLE_SPELLCHECK
	qputenv("OTTER_DICTIONARIES", SessionsManager::getWritableDataPath(QLatin1String("dictionaries")).toLatin1());

	m_dictionaries = Sonnet::Speller().availableDictionaries();
#endif
}

void SpellCheckManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new SpellCheckManager(QCoreApplication::instance());
	}
}

void SpellCheckManager::updateDefaultDictionary()
{
	const QStringList dictionaries(m_dictionaries.values());
	const QString defaultLanguage(QLocale().bcp47Name());

	if (dictionaries.contains(defaultLanguage))
	{
		m_defaultDictionary = defaultLanguage;

		return;
	}

	const QLocale locale(defaultLanguage);
	const QList<QLocale> locales(QLocale::matchingLocales(locale.language(), locale.script(), QLocale::AnyCountry));

	for (int i = 0; i < locales.count(); ++i)
	{
		if (dictionaries.contains(locales.at(i).name()))
		{
			m_defaultDictionary = locales.at(i).name();

			break;
		}

		if (dictionaries.contains(locales.at(i).bcp47Name()))
		{
			m_defaultDictionary = locales.at(i).bcp47Name();

			break;
		}
	}

	if (m_defaultDictionary.isEmpty())
	{
		m_defaultDictionary = QLatin1String("en_US");
	}
}

SpellCheckManager* SpellCheckManager::getInstance()
{
	return m_instance;
}

QString SpellCheckManager::getDefaultDictionary()
{
	if (m_defaultDictionary.isEmpty())
	{
		updateDefaultDictionary();
	}

	return m_defaultDictionary;
}

QVector<SpellCheckManager::DictionaryInformation> SpellCheckManager::getDictionaries()
{
	QVector<DictionaryInformation> dictionaries;
	dictionaries.reserve(m_dictionaries.count());

	QMap<QString, QString>::const_iterator iterator;

	for (iterator = m_dictionaries.constBegin(); iterator != m_dictionaries.constEnd(); ++iterator)
	{
		DictionaryInformation dictionary;
		dictionary.name = iterator.value();
		dictionary.title = iterator.key();

		dictionaries.append(dictionary);
	}

	return dictionaries;
}

bool SpellCheckManager::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		updateDefaultDictionary();
	}

	return QObject::event(event);
}

}
