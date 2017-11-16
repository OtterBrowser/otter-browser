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

namespace Otter
{

class SpellCheckManager final : public QObject
{
	Q_OBJECT

public:
	struct DictionaryInformation final
	{
		QString name;
		QString title;
	};

	static void createInstance();
	static SpellCheckManager* getInstance();
	static QString getDefaultDictionary();
	static QVector<DictionaryInformation> getDictionaries();
	bool event(QEvent *event) override;

protected:
	explicit SpellCheckManager(QObject *parent);

	static void updateDefaultDictionary();

private:
	static SpellCheckManager *m_instance;
	static QString m_defaultDictionary;
	static QMap<QString, QString> m_dictionaries;
};

}

#endif
