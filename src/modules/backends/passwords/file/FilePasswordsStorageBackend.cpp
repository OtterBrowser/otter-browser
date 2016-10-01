/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	QFile file(SessionsManager::getWritableDataPath(QLatin1String("passwords.json")));

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(tr("Failed to open passwords file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QHash<QString, QList<PasswordsManager::PasswordInformation> > passwords;
	QJsonObject hostsObject(QJsonDocument::fromJson(file.readAll()).object());
	QJsonObject::const_iterator hostsIterator;

	for (hostsIterator = hostsObject.constBegin(); hostsIterator != hostsObject.constEnd(); ++hostsIterator)
	{
		const QJsonArray hostArray(hostsIterator.value().toArray());
		QList<PasswordsManager::PasswordInformation> hostPasswords;

		for (int i = 0; i < hostArray.count(); ++i)
		{
			const QJsonObject passwordObject(hostArray.at(i).toObject());
			PasswordsManager::PasswordInformation password;
			password.url = QUrl(passwordObject.value(QLatin1String("url")).toString());
			password.type = ((passwordObject.value(QLatin1String("type")).toString() == QLatin1String("auth")) ? PasswordsManager::AuthPassword : PasswordsManager::FormPassword);

			const QJsonArray fieldsArray(passwordObject.value(QLatin1String("fields")).toArray());

			for (int j = 0; j < fieldsArray.count(); ++j)
			{
				const QJsonObject fieldObject(fieldsArray.at(j).toObject());

				password.fields.append(qMakePair(fieldObject.value(QLatin1String("key")).toString(), fieldObject.value(QLatin1String("value")).toString()));

				if (fieldObject.value(QLatin1String("type")).toString() == QLatin1String("password"))
				{
					password.passwords.append(fieldObject.value(QLatin1String("key")).toString());
				}
			}

			hostPasswords.append(password);
		}

		passwords[hostsIterator.key()] = hostPasswords;
	}

	m_passwords = passwords;

	emit passwordsModified();
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
	QHash<QString, QList<PasswordsManager::PasswordInformation> >::iterator hostsIterator;

	for (hostsIterator = m_passwords.begin(); hostsIterator != m_passwords.end(); ++hostsIterator)
	{
		QJsonArray hostArray;
		const QList<PasswordsManager::PasswordInformation> passwords(hostsIterator.value());

		for (int i = 0; i < passwords.count(); ++i)
		{
			QJsonArray fieldsArray;

			for (int j = 0; j < passwords.at(i).fields.count(); ++j)
			{
				QJsonObject fieldObject;
				fieldObject.insert(QLatin1String("key"), passwords.at(i).fields.at(j).first);
				fieldObject.insert(QLatin1String("value"), passwords.at(i).fields.at(j).second);
				fieldObject.insert(QLatin1String("type"), (passwords.at(i).passwords.contains(passwords.at(i).fields.at(j).first) ? QLatin1String("password") : QLatin1String("text")));

				fieldsArray.append(fieldObject);
			}

			QJsonObject passwordObject;
			passwordObject.insert(QLatin1String("url"), passwords.at(i).url.toString());
			passwordObject.insert(QLatin1String("type"), ((passwords.at(i).type == PasswordsManager::AuthPassword) ? QLatin1String("auth") : QLatin1String("form")));
			passwordObject.insert(QLatin1String("fields"), fieldsArray);

			hostArray.append(passwordObject);
		}

		hostsObject.insert(hostsIterator.key(), hostArray);
	}

	file.write(QJsonDocument(hostsObject).toJson(QJsonDocument::Indented));
	file.close();
}

void FilePasswordsStorageBackend::clearPasswords(const QString &host)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	if (m_passwords.isEmpty())
	{
		return;
	}

	if (host.isEmpty())
	{
		const QString path(SessionsManager::getWritableDataPath(QLatin1String("passwords.json")));

		if (QFile::remove(path))
		{
			m_passwords.clear();

			emit passwordsModified();
		}
		else
		{
			Console::addMessage(tr("Failed to remove passwords file"), Console::OtherCategory, Console::ErrorLevel, path);
		}

		return;
	}

	if (m_passwords.contains(host))
	{
		m_passwords.remove(host);

		emit passwordsModified();

		save();
	}
}

void FilePasswordsStorageBackend::addPassword(const PasswordsManager::PasswordInformation &password)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(password.url.host().isEmpty() ? QLatin1String("localhost") : password.url.host());

	if (!m_passwords.contains(host))
	{
		m_passwords[host] = QList<PasswordsManager::PasswordInformation>();
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

	const QString host(password.url.host().isEmpty() ? QLatin1String("localhost") : password.url.host());

	if (!m_passwords.contains(host))
	{
		return;
	}

	const QList<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);

	for (int i = 0; i < passwords.count(); ++i)
	{
		if (passwords.at(i).type == password.type && passwords.at(i).url == password.url && passwords.at(i).passwords == password.passwords && passwords.at(i).fields.count() == password.fields.count())
		{
			bool isMatching(true);

			for (int j = 0; j < password.fields.count(); ++j)
			{
				if (passwords.at(i).fields.at(j).first != password.fields.at(j).first || (!passwords.at(i).passwords.contains(passwords.at(i).fields.at(j).first) && passwords.at(i).fields.at(j).second != password.fields.at(j).second))
				{
					isMatching = false;

					break;
				}
			}

			if (isMatching)
			{
				m_passwords[host].removeAt(i);

				emit passwordsModified();

				save();

				break;
			}
		}
	}
}

QString FilePasswordsStorageBackend::getTitle() const
{
	return QString(tr("Encrypted File"));
}

QString FilePasswordsStorageBackend::getDescription() const
{
	return QString(tr("Stores passwords in AES encrypted file."));
}

QString FilePasswordsStorageBackend::getVersion() const
{
	return QLatin1String("1.0");
}

QUrl FilePasswordsStorageBackend::getHomePage() const
{
	return QUrl(QLatin1String("http://otter-browser.org/"));
}

QIcon FilePasswordsStorageBackend::getIcon() const
{
	return QIcon();
}

QStringList FilePasswordsStorageBackend::getHosts()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_passwords.keys();
}

QList<PasswordsManager::PasswordInformation> FilePasswordsStorageBackend::getPasswords(const QUrl &url, PasswordsManager::PasswordTypes types)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(url.host().isEmpty() ? QLatin1String("localhost") : url.host());

	if (m_passwords.contains(host))
	{
		if (types == PasswordsManager::AnyPassword)
		{
			return m_passwords[host];
		}

		const QList<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);
		QList<PasswordsManager::PasswordInformation> matchingPasswords;

		for (int i = 0; i < passwords.count(); ++i)
		{
			if (types.testFlag(passwords.at(i).type))
			{
				matchingPasswords.append(passwords.at(i));
			}
		}

		return matchingPasswords;
	}

	return QList<PasswordsManager::PasswordInformation>();
}

bool FilePasswordsStorageBackend::hasPasswords(const QUrl &url, PasswordsManager::PasswordTypes types)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	const QString host(url.host().isEmpty() ? QLatin1String("localhost") : url.host());

	if (types == PasswordsManager::AnyPassword)
	{
		return m_passwords.contains(host);
	}

	if (m_passwords.contains(host))
	{
		const QList<PasswordsManager::PasswordInformation> passwords(m_passwords[host]);

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
