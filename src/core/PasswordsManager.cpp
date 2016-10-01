/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PasswordsManager.h"
#include "PasswordsStorageBackend.h"
#include "../modules/backends/passwords/file/FilePasswordsStorageBackend.h"

namespace Otter
{

PasswordsManager* PasswordsManager::m_instance = NULL;
PasswordsStorageBackend* PasswordsManager::m_backend = NULL;

PasswordsManager::PasswordsManager(QObject *parent) : QObject(parent)
{
}

void PasswordsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new PasswordsManager(parent);
		m_backend = new FilePasswordsStorageBackend(m_instance);

		connect(m_backend, SIGNAL(passwordsModified()), m_instance, SIGNAL(passwordsModified()));
	}
}

void PasswordsManager::addPassword(const PasswordInformation &password)
{
	if (m_backend)
	{
		m_backend->addPassword(password);
	}
}

void PasswordsManager::removePassword(const PasswordsManager::PasswordInformation &password)
{
	if (m_backend)
	{
		m_backend->removePassword(password);
	}
}

PasswordsManager* PasswordsManager::getInstance()
{
	return m_instance;
}

QList<PasswordsManager::PasswordInformation> PasswordsManager::getPasswords(const QUrl &url, PasswordTypes types)
{
	return (m_backend ? m_backend->getPasswords(url, types) : QList<PasswordsManager::PasswordInformation>());
}

bool PasswordsManager::hasPasswords(const QUrl &url, PasswordTypes types)
{
	return (m_backend ? m_backend->hasPasswords(url, types) : false);
}

}
