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

#ifndef OTTER_SPELLCHECKMANAGER_H
#define OTTER_SPELLCHECKMANAGER_H

#include <QtCore/QObject>

#ifdef OTTER_ENABLE_SPELLCHECK
#include "../../3rdparty/sonnet/src/core/speller.h"
#endif

namespace Otter
{

class SpellCheckManager : public QObject
{
	Q_OBJECT

public:
	struct DictionaryInformation
	{
		QString name;
		QString title;
	};

	~SpellCheckManager();

	static void createInstance(QObject *parent = nullptr);
	static SpellCheckManager* getInstance();
	static QString getDefaultDictionary();
	static QList<DictionaryInformation> getDictionaries();
	bool event(QEvent *event) override;

protected:
	explicit SpellCheckManager(QObject *parent = nullptr);

	static void updateDefaultDictionary();

private:
	static SpellCheckManager *m_instance;
#ifdef OTTER_ENABLE_SPELLCHECK
	static Sonnet::Speller *m_speller;
#endif
	static QString m_defaultDictionary;
};

}

#endif
