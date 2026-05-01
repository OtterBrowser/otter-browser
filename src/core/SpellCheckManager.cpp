/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2026 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
QVector<SpellCheckManager::Dictionary> SpellCheckManager::m_dictionaries;
QSet<QString> SpellCheckManager::m_ignoredWords;

SpellCheckManager::SpellCheckManager(QObject *parent) : QObject(parent)
{
#ifdef OTTER_ENABLE_SPELLCHECK
	qputenv("OTTER_DICTIONARIES", getDictionariesPath().toLatin1());
#endif
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, [&](int identifier)
	{
		if (identifier == SettingsManager::Browser_SpellCheckIgnoreDctionariesOption)
		{
			loadDictionaries();
		}
	});
}

void SpellCheckManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new SpellCheckManager(QCoreApplication::instance());

		loadDictionaries();
	}
}

void SpellCheckManager::addIgnoredWord(const QString &word)
{
	if (!isIgnoringWord(word))
	{
		m_ignoredWords.insert(word);

		emit m_instance->ignoredWordAdded(word);

		saveIgnoredWords();
	}
}

void SpellCheckManager::removeIgnoredWord(const QString &word)
{
	if (isIgnoringWord(word))
	{
		m_ignoredWords.remove(word);

		emit m_instance->ignoredWordRemoved(word);

		saveIgnoredWords();
	}
}

void SpellCheckManager::loadDictionaries()
{
#ifdef OTTER_ENABLE_SPELLCHECK
	const QVector<Sonnet::Speller::Dictionary> dictionaries(Sonnet::Speller().availableDictionaries());

	m_dictionaries.clear();
	m_dictionaries.reserve(dictionaries.count());

	const QDir dictionariesDirectory(getDictionariesPath());
	const QStringList ignoredDictionaries(SettingsManager::getOption(SettingsManager::Browser_SpellCheckIgnoreDctionariesOption).toStringList());

	for (const Sonnet::Speller::Dictionary &dictionary: dictionaries)
	{
		if (ignoredDictionaries.contains(dictionary.langCode))
		{
			continue;
		}

		Dictionary information;
		information.language = dictionary.langCode;
		information.title = dictionary.name;
		information.paths = dictionary.paths;

		for (int i = 0; i < information.paths.count(); ++i)
		{
			const QFileInfo pathInformation(information.paths.at(i));

			if (pathInformation.isRelative())
			{
				information.paths[i] = pathInformation.absoluteFilePath();
			}

			if (pathInformation.dir() != dictionariesDirectory)
			{
				information.isLocalDictionary = false;
			}
		}

		m_dictionaries.append(information);
	}

	if (!m_defaultDictionary.isEmpty())
	{
		updateDefaultDictionary();
	}

	emit m_instance->dictionariesChanged();
#endif
}

void SpellCheckManager::saveIgnoredWords()
{
	const QString path(SessionsManager::getWritableDataPath(QLatin1String("personalDictionary.dic")));

	if (m_ignoredWords.isEmpty() && QFile::exists(path))
	{
		QFile::remove(path);

		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::WriteOnly))
	{
		return;
	}

	QTextStream stream(&file);
	QSet<QString>::iterator iterator;

	for (iterator = m_ignoredWords.begin(); iterator != m_ignoredWords.end(); ++iterator)
	{
		stream << *iterator << QLatin1Char('\n');
	}
}

void SpellCheckManager::updateDefaultDictionary()
{
	const QString fallbackDictionary(QLatin1String("en_US"));

	if (m_dictionaries.isEmpty())
	{
		m_defaultDictionary = fallbackDictionary;

		return;
	}

	QStringList dictionaries;
	dictionaries.reserve(m_dictionaries.count());

	for (const Dictionary &dictionary: std::as_const(m_dictionaries))
	{
		dictionaries.append(dictionary.language);
	}

	const QString defaultLanguage(QLocale().bcp47Name());

	if (dictionaries.contains(defaultLanguage))
	{
		m_defaultDictionary = defaultLanguage;

		return;
	}

	const QLocale defaultLocale(defaultLanguage);
	const QList<QLocale> locales(QLocale::matchingLocales(defaultLocale.language(), defaultLocale.script(), QLocale::AnyCountry));

	for (const QLocale &locale: locales)
	{
		const QString localeName(locale.name());

		if (dictionaries.contains(localeName))
		{
			m_defaultDictionary = localeName;

			break;
		}

		const QString localeBcp47Name(locale.bcp47Name());

		if (dictionaries.contains(localeBcp47Name))
		{
			m_defaultDictionary = localeBcp47Name;

			break;
		}
	}

	if (m_defaultDictionary.isEmpty())
	{
		m_defaultDictionary = fallbackDictionary;
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

QString SpellCheckManager::getDictionariesPath()
{
	QString dictionariesPath(SessionsManager::getWritableDataPath(QLatin1String("dictionaries")));

	if (!QFile::exists(dictionariesPath))
	{
		dictionariesPath = QDir::toNativeSeparators(Application::getApplicationDirectoryPath() + QDir::separator() + QLatin1String("dictionaries"));;
	}

	return dictionariesPath;
}

SpellCheckManager::Dictionary SpellCheckManager::getDictionary(const QString &language)
{
	for (const Dictionary &dictionary: std::as_const(m_dictionaries))
	{
		if (dictionary.language == language)
		{
			return dictionary;
		}
	}

	return {};
}

QVector<SpellCheckManager::Dictionary> SpellCheckManager::getDictionaries()
{
	return m_dictionaries;
}

QStringList SpellCheckManager::getIgnoredWords()
{
	const QString path(SessionsManager::getWritableDataPath(QLatin1String("personalDictionary.dic")));

	if (m_ignoredWords.isEmpty() && QFile::exists(path))
	{
		QFile file(path);

		if (file.open(QIODevice::ReadOnly))
		{
			QTextStream stream(&file);

			while (!stream.atEnd())
			{
				const QString word(stream.readLine().simplified());

				if (!word.isEmpty())
				{
					m_ignoredWords.insert(word);
				}
			}
		}
	}

	return m_ignoredWords.values();
}

bool SpellCheckManager::isIgnoringWord(const QString &word)
{
	return getIgnoredWords().contains(word);
}

bool SpellCheckManager::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		updateDefaultDictionary();
	}

	return QObject::event(event);
}

Dictionary::Dictionary(const SpellCheckManager::Dictionary &information, QObject *parent) : QObject(parent),
	m_information(information)
{
}

QString Dictionary::getLanguage() const
{
	return m_information.language;
}

QString Dictionary::getName() const
{
	return m_information.language;
}

QString Dictionary::getTitle() const
{
	return m_information.title;
}

QStringList Dictionary::getPaths() const
{
	return m_information.paths;
}

Addon::AddonType Dictionary::getType() const
{
	return DictionaryType;
}

bool Dictionary::isEnabled() const
{
	return !SettingsManager::getOption(SettingsManager::Browser_SpellCheckIgnoreDctionariesOption).toStringList().contains(m_information.language);
}

bool Dictionary::canRemove() const
{
	bool canRemove(true);

	for (const QString &path: m_information.paths)
	{
		if (!QFileInfo(path).isWritable())
		{
			canRemove = false;
		}
	}

	return canRemove;
}

bool Dictionary::remove()
{
	if (!canRemove())
	{
		return false;
	}

	for (const QString &path: m_information.paths)
	{
		QFile::remove(path);
	}

	return true;
}

}
