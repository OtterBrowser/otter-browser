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

#ifndef OTTER_SPELLCHECKMANAGER_H
#define OTTER_SPELLCHECKMANAGER_H

#include "AddonsManager.h"

namespace Otter
{

class SpellCheckManager final : public QObject
{
	Q_OBJECT

public:
	struct DictionaryInformation final
	{
		QString language;
		QString title;
		QStringList paths;
		bool isLocalDictionary = true;

		bool isValid() const
		{
			return (!language.isEmpty() && !paths.isEmpty());
		}
	};

	static void createInstance();
	static void addIgnoredWord(const QString &word);
	static void removeIgnoredWord(const QString &word);
	static SpellCheckManager* getInstance();
	static QString getDefaultDictionary();
	static QString getDictionariesPath();
	static DictionaryInformation getDictionary(const QString &language);
	static QVector<DictionaryInformation> getDictionaries();
	static QStringList getIgnoredWords();
	static bool isIgnoringWord(const QString &word);
	bool event(QEvent *event) override;

protected:
	explicit SpellCheckManager(QObject *parent);

	static void updateDefaultDictionary();
	static void loadDictionaries();
	static void saveIgnoredWords();

private:
	static SpellCheckManager *m_instance;
	static QString m_defaultDictionary;
	static QVector<DictionaryInformation> m_dictionaries;
	static QSet<QString> m_ignoredWords;

signals:
	void dictionariesChanged();
	void ignoredWordAdded(const QString &word);
	void ignoredWordRemoved(const QString &word);
};

class Dictionary final : public QObject, public Addon
{
public:
	explicit Dictionary(const SpellCheckManager::DictionaryInformation &information, QObject *parent);

	QString getLanguage() const;
	QString getName() const override;
	QString getTitle() const override;
	QStringList getPaths() const;
	AddonType getType() const override;
	bool isEnabled() const override;
	bool canRemove() const override;
	bool remove() override;

private:
	SpellCheckManager::DictionaryInformation m_information;
};

}

#endif
