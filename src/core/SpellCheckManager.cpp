/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Application.h"
#include "SessionsManager.h"
#ifdef OTTER_ENABLE_SPELLCHECK
#include "../../3rdparty/sonnet/src/core/speller.h"
#endif

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QLocale>

namespace Otter
{

SpellCheckManager* SpellCheckManager::m_instance(nullptr);
QString SpellCheckManager::m_defaultDictionary;
QVector<SpellCheckManager::DictionaryInformation> SpellCheckManager::m_dictionaries;

SpellCheckManager::SpellCheckManager(QObject *parent) : QObject(parent)
{
#ifdef OTTER_ENABLE_SPELLCHECK
	QString dictionariesPath(SessionsManager::getWritableDataPath(QLatin1String("dictionaries")));

	if (!QFile::exists(dictionariesPath))
	{
		dictionariesPath = QDir::toNativeSeparators(Application::getApplicationDirectoryPath() + QDir::separator() + QLatin1String("dictionaries"));;
	}

	qputenv("OTTER_DICTIONARIES", dictionariesPath.toLatin1());

	const QVector<Sonnet::Speller::Dictionary> dictionaries(Sonnet::Speller().availableDictionaries());

	m_dictionaries.reserve(dictionaries.count());

	for (int i = 0; i < dictionaries.count(); ++i)
	{
		Sonnet::Speller::Dictionary dictionary(dictionaries.at(i));
		DictionaryInformation information;
		information.name = dictionary.langCode;
		information.title = dictionary.name;
		information.paths = dictionary.paths;

		m_dictionaries.append(information);
	}
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
	QStringList dictionaries;
	dictionaries.reserve(m_dictionaries.count());

	for (int i = 0; i < m_dictionaries.count(); ++i)
	{
		dictionaries.append(m_dictionaries.at(i).name);
	}

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
	return m_dictionaries;
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
