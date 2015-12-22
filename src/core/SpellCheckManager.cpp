/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SpellCheckManager.h"

namespace Otter
{

SpellCheckManager* SpellCheckManager::m_instance = NULL;
QString SpellCheckManager::m_defaultDictionary;

SpellCheckManager::SpellCheckManager(QObject *parent) : QObject(parent)
{
}

void SpellCheckManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SpellCheckManager(parent);
	}
}

void SpellCheckManager::setDefaultDictionary(const QString &dictionary)
{
	if (dictionary != m_defaultDictionary)
	{
		m_defaultDictionary = dictionary;

		emit m_instance->defaultDictionaryChanged(dictionary);
	}
}

SpellCheckManager* SpellCheckManager::getInstance()
{
	return m_instance;
}

QString SpellCheckManager::getDefaultDictionary()
{
	return m_defaultDictionary;
}

QList<SpellCheckManager::DictionaryInformation> SpellCheckManager::getDictonaries()
{
///TODO

	return QList<DictionaryInformation>();
}

}
