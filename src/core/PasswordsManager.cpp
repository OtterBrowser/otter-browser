/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PasswordsManager.h"
#include "PasswordsStorageBackend.h"
#include "../modules/backends/passwords/file/FilePasswordsStorageBackend.h"

namespace Otter
{

PasswordsManager* PasswordsManager::m_instance(nullptr);
PasswordsStorageBackend* PasswordsManager::m_backend(nullptr);

PasswordsManager::PasswordsManager(QObject *parent) : QObject(parent)
{
}

void PasswordsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new PasswordsManager(QCoreApplication::instance());
		m_backend = new FilePasswordsStorageBackend(m_instance);

		connect(m_backend, &PasswordsStorageBackend::passwordsModified, m_instance, &PasswordsManager::passwordsModified);
	}
}

void PasswordsManager::clearPasswords(const QString &host)
{
	if (m_backend)
	{
		m_backend->clearPasswords(host);
	}
}

void PasswordsManager::clearPasswords(int period)
{
	if (m_backend)
	{
		m_backend->clearPasswords(period);
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

QStringList PasswordsManager::getHosts()
{
	return (m_backend ? m_backend->getHosts() : QStringList());
}

QVector<PasswordsManager::PasswordInformation> PasswordsManager::getPasswords(const QUrl &url, PasswordTypes types)
{
	return (m_backend ? m_backend->getPasswords(url, types) : QVector<PasswordsManager::PasswordInformation>());
}

PasswordsManager::PasswordMatch PasswordsManager::hasPassword(const PasswordsManager::PasswordInformation &password)
{
	return (m_backend ? m_backend->hasPassword(password) : NoMatch);
}

bool PasswordsManager::hasPasswords(const QUrl &url, PasswordTypes types)
{
	return (m_backend ? m_backend->hasPasswords(url, types) : false);
}

}
