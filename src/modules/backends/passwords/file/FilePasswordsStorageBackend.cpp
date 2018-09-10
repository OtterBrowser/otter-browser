/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "FilePasswordsStorageBackend.h"
#include "../../../../core/Console.h"
#include "../../../../core/SessionsManager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Otter
{

FilePasswordsStorageBackend::FilePasswordsStorageBackend(QObject *parent) : PasswordsStorageBackend(parent),
	m_isInitialized(false)
{
}

void FilePasswordsStorageBackend::initialize()
{
	m_isInitialized = true;

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("passwords.json")));

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(tr("Failed to open passwords file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QHash<QString, QVector<PasswordsManager::PasswordInformation> > passwords;
	QJsonObject hostsObject(QJsonDocument::fromJson(file.readAll()).object());
	QJsonObject::const_iterator hostsIterator;

	for (hostsIterator = hostsObject.constBegin(); hostsIterator != hostsObject.constEnd(); ++hostsIterator)
	{
		const QJsonArray hostArray(hostsIterator.value().toArray());
		QVector<PasswordsManager::PasswordInformation> hostPasswords;
		hostPasswords.reserve(hostArray.count());

		for (int i = 0; i < hostArray.count(); ++i)
		{
			const QJsonObject passwordObject(hostArray.at(i).toObject());
			PasswordsManager::PasswordInformation password;
			password.url = QUrl(passwordObject.value(QLatin1String("url")).toString());
			password.timeAdded = QDateTime::fromString(passwordObject.value(QLatin1String("timeAdded")).toString(), Qt::ISODate);
			password.timeAdded.setTimeSpec(Qt::UTC);
			password.timeUsed = QDateTime::fromString(passwordObject.value(QLatin1String("timeUsed")).toString(), Qt::ISODate);
			password.timeUsed.setTimeSpec(Qt::UTC);
			password.type = ((passwordObject.value(QLatin1String("type")).toString() == QLatin1String("auth")) ? PasswordsManager::AuthPassword : PasswordsManager::FormPassword);

			const QJsonArray fieldsArray(passwordObject.value(QLatin1String("fields")).toArray());

			password.fields.reserve(fieldsArray.count());

			for (int j = 0; j < fieldsArray.count(); ++j)
			{
				const QJsonObject fieldObject(fieldsArray.at(j).toObject());
				PasswordsManager::PasswordInformation::Field field;
				field.name = fieldObject.value(fieldObject.contains(QLatin1String("name")) ? QLatin1String("name") : QLatin1String("key")).toString();
				field.value = fieldObject.value(QLatin1String("value")).toString();
				field.type = ((fieldObject.value(QLatin1String("type")).toString() == QLatin1String("password")) ? PasswordsManager::PasswordField : PasswordsManager::TextField);

				password.fields.append(field);
			}

			hostPasswords.append(password);
		}

		passwords[hostsIterator.key()] = hostPasswords;
	}

	m_passwords = passwords;
}

void FilePasswordsStorageBackend::save()
{
	QFile file(SessionsManager::getWritableDataPath(QLatin1String("passwords.json")));

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		Console::addMessage(tr("Failed to save passwords file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QJsonObject hostsObject;
	QHash<QString, QVector<PasswordsManager::PasswordInformation> >::iterator hostsIterator;

	for (hostsIterator = m_passwords.begin(); hostsIterator != m_passwords.end(); ++hostsIterator)
	{
		if (hostsIterator.value().isEmpty())
		{
			continue;
		}

		QJsonArray hostArray;
		const QVector<PasswordsManager::PasswordInformation> passwords(hostsIterator.value());

		for (int i = 0; i < passwords.count(); ++i)
		{
			QJsonArray fieldsArray;

			for (int j = 0; j < passwords.at(i).fields.count(); ++j)
			{
				fieldsArray.append(QJsonObject({{QLatin1String("name"), passwords.at(i).fields.at(j).name}, {QLatin1String("value"), passwords.at(i).fields.at(j).value}, {QLatin1String("type"), ((passwords.at(i).fields.at(j).type == PasswordsManager::PasswordField) ? QLatin1String("password") : QLatin1String("text"))}}));
			}

			QJsonObject passwordObject({{QLatin1String("url"), passwords.at(i).url.toString()}});

			if (passwords.at(i).timeAdded.isValid())
			{
				passwordObject.insert(QLatin1String("timeAdded"), passwords.at(i).timeAdded.toString(Qt::ISODate));
			}

			if (passwords.at(i).timeUsed.isValid())
			{
				passwordObject.insert(QLatin1String("timeUsed"), passwords.at(i).timeUsed.toString(Qt::ISODate));
			}

			passwordObject.insert(QLatin1String("type"), ((passwords.at(i).type == PasswordsManager::AuthPassword) ? QLatin1String("auth") : QLatin1String("form")));
			passwordObject.insert(QLatin1String("fields"), fieldsArray);

			hostArray.append(passwordObject);
		}

		hostsObject.insert(hostsIterator.key(), hostArray);
	}

	file.write(QJsonDocument(hostsObject).toJson(QJsonDocument::Compact));
	file.close();
}

void FilePasswordsStorageBackend::clearPasswords(const QString &host)
{
	if (host.isEmpty())
	{
		return;
	}

	if (!m_isInitialized)
	{
		initialize();
	}

	if (m_passwords.isEmpty())
	{
		return;
	}

	if (m_passwords.contains(host))
	{
		m_passwords.remove(host);

		emit passwordsModified();

		save();
	}
}

void FilePasswordsStorageBackend::clearPasswords(int period)
{
	if (period <= 0)
	{
		const QString path(SessionsManager::getWritableDataPath(QLatin1String("passwords.json")));

		if (!QFile::remove(path))
		{
			Console::addMessage(tr("Failed to remove passwords file"), Console::OtherCategory, Console::ErrorLevel, path);
		}
		else if (!m_passwords.isEmpty())
		{
			m_passwords.clear();

			emit passwordsModified();
		}
	}

	if (!m_isInitialized)
	{
		initialize();
	}

	if (m_passwords.isEmpty())
	{
		return;
	}

	QHash<QString, QVector<PasswordsManager::PasswordInformation> >::iterator iterator(m_passwords.begin());
	bool wasModified(false);

	while (iterator != m_passwords.end())
	{
		QVector<PasswordsManager::PasswordInformation> passwords(iterator.value());

		for (int i = (passwords.count() - 1); i >= 0; --i)
		{
			if (passwords.at(i).timeAdded.secsTo(QDateTime::currentDateTimeUtc()) < (period * 3600))
			{
				passwords.removeAt(i);

				wasModified = true;
			}
		}

		if (passwords.isEmpty())
		{
			iterator = m_passwords.erase(iterator);
		}
		else
		{
			m_passwords[iterator.key()] = passwords;

			++iterator;
		}
	}

	if (wasModified)
	{
		save();

		emit passwordsModified();
	}
}

void FilePasswordsStorageBackend::addPassword(const PasswordsManager::PasswordInformation &password)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(Utils::extractHost(password.url));

	if (m_passwords.contains(host))
	{
		const QVector<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);

		for (int i = 0; i < passwords.count(); ++i)
		{
			if (comparePasswords(password, passwords.at(i)) == PasswordsManager::FullMatch)
			{
				return;
			}

			if (comparePasswords(password, passwords.at(i)) == PasswordsManager::PartialMatch)
			{
				m_passwords[host].replace(i, password);

				emit passwordsModified();

				save();

				return;
			}
		}
	}
	else
	{
		m_passwords[host] = QVector<PasswordsManager::PasswordInformation>();
	}

	m_passwords[host].append(password);

	emit passwordsModified();

	save();
}

void FilePasswordsStorageBackend::removePassword(const PasswordsManager::PasswordInformation &password)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(Utils::extractHost(password.url));

	if (!m_passwords.contains(host))
	{
		return;
	}

	const QVector<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);

	for (int i = 0; i < passwords.count(); ++i)
	{
		if (comparePasswords(password, passwords.at(i)) != PasswordsManager::NoMatch)
		{
			m_passwords[host].removeAt(i);

			emit passwordsModified();

			save();

			return;
		}
	}
}

QString FilePasswordsStorageBackend::getTitle() const
{
	return tr("Encrypted File");
}

QString FilePasswordsStorageBackend::getDescription() const
{
	return tr("Stores passwords in AES encrypted file.");
}

QString FilePasswordsStorageBackend::getVersion() const
{
	return QLatin1String("1.0");
}

QUrl FilePasswordsStorageBackend::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList FilePasswordsStorageBackend::getHosts()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_passwords.keys();
}

QVector<PasswordsManager::PasswordInformation> FilePasswordsStorageBackend::getPasswords(const QUrl &url, PasswordsManager::PasswordTypes types)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(Utils::extractHost(url));

	if (m_passwords.contains(host))
	{
		if (types == PasswordsManager::AnyPassword)
		{
			return m_passwords[host];
		}

		const QVector<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);
		QVector<PasswordsManager::PasswordInformation> matchingPasswords;

		for (int i = 0; i < passwords.count(); ++i)
		{
			if (types.testFlag(passwords.at(i).type))
			{
				matchingPasswords.append(passwords.at(i));
			}
		}

		return matchingPasswords;
	}

	return {};
}

PasswordsManager::PasswordMatch FilePasswordsStorageBackend::hasPassword(const PasswordsManager::PasswordInformation &password)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(Utils::extractHost(password.url));

	if (!m_passwords.contains(host))
	{
		return PasswordsManager::NoMatch;
	}

	const QVector<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);

	for (int i = 0; i < passwords.count(); ++i)
	{
		const PasswordsManager::PasswordMatch match(comparePasswords(password, passwords.at(i)));

		if (match != PasswordsManager::NoMatch)
		{
			return match;
		}
	}

	return PasswordsManager::NoMatch;
}

bool FilePasswordsStorageBackend::hasPasswords(const QUrl &url, PasswordsManager::PasswordTypes types)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(Utils::extractHost(url));

	if (types == PasswordsManager::AnyPassword)
	{
		return m_passwords.contains(host);
	}

	if (m_passwords.contains(host))
	{
		const QVector<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);

		for (int i = 0; i < passwords.count(); ++i)
		{
			if (types.testFlag(passwords.at(i).type))
			{
				return true;
			}
		}
	}

	return false;
}

}
