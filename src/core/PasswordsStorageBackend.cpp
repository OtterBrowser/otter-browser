/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PasswordsStorageBackend.h"

namespace Otter
{

PasswordsStorageBackend::PasswordsStorageBackend(QObject *parent) : QObject(parent)
{
}

void PasswordsStorageBackend::clearPasswords(const QString &host)
{
	Q_UNUSED(host)
}

void PasswordsStorageBackend::clearPasswords(int period)
{
	Q_UNUSED(period)
}

void PasswordsStorageBackend::addPassword(const PasswordsManager::PasswordInformation &password)
{
	Q_UNUSED(password)
}

void PasswordsStorageBackend::removePassword(const PasswordsManager::PasswordInformation &password)
{
	Q_UNUSED(password)
}

QStringList PasswordsStorageBackend::getHosts()
{
	return {};
}

QVector<PasswordsManager::PasswordInformation> PasswordsStorageBackend::getPasswords(const QUrl &url, PasswordsManager::PasswordTypes types)
{
	Q_UNUSED(url)
	Q_UNUSED(types)

	return {};
}

Addon::AddonType PasswordsStorageBackend::getType() const
{
	return PasswordsStorageBackendType;
}

PasswordsManager::PasswordMatch PasswordsStorageBackend::comparePasswords(const PasswordsManager::PasswordInformation &first, const PasswordsManager::PasswordInformation &second)
{
	if (first.type != second.type || first.url != second.url || first.fields.count() != second.fields.count())
	{
		return PasswordsManager::NoMatch;
	}

	PasswordsManager::PasswordMatch match(PasswordsManager::FullMatch);

	for (int i = 0; i < first.fields.count(); ++i)
	{
		const PasswordsManager::PasswordInformation::Field firstField(first.fields.at(i));
		const PasswordsManager::PasswordInformation::Field secondField(second.fields.at(i));

		if (firstField.name != secondField.name || firstField.type != secondField.type)
		{
			return PasswordsManager::NoMatch;
		}

		if (firstField.value != secondField.value)
		{
			if (firstField.type != PasswordsManager::PasswordField)
			{
				return PasswordsManager::NoMatch;
			}

			match = PasswordsManager::PartialMatch;
		}
	}

	return match;
}

PasswordsManager::PasswordMatch PasswordsStorageBackend::hasPassword(const PasswordsManager::PasswordInformation &password)
{
	Q_UNUSED(password)

	return PasswordsManager::NoMatch;
}

bool PasswordsStorageBackend::hasPasswords(const QUrl &url, PasswordsManager::PasswordTypes types)
{
	Q_UNUSED(url)
	Q_UNUSED(types)

	return false;
}

}
