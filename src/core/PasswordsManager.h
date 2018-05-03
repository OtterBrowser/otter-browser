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

#ifndef OTTER_PASSWORDSMANAGER_H
#define OTTER_PASSWORDSMANAGER_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVector>

namespace Otter
{

class PasswordsStorageBackend;

class PasswordsManager final : public QObject
{
	Q_OBJECT

public:
	enum FieldType
	{
		UnknownField = 0,
		PasswordField,
		TextField
	};

	enum PasswordMatch
	{
		NoMatch = 0,
		PartialMatch,
		FullMatch
	};

	enum PasswordType
	{
		UnknownPassword = 0,
		FormPassword = 1,
		AuthPassword = 2,
		AnyPassword = (FormPassword | AuthPassword)
	};

	Q_DECLARE_FLAGS(PasswordTypes, PasswordType)

	struct PasswordInformation final
	{
		struct Field final
		{
			QString name;
			QString value;
			FieldType type = UnknownField;
		};

		QUrl url;
		QDateTime timeAdded;
		QDateTime timeUsed;
		QVector<Field> fields;
		PasswordType type = FormPassword;

		bool isValid() const
		{
			return !fields.isEmpty();
		}
	};

	static void createInstance();
	static void clearPasswords(const QString &host);
	static void clearPasswords(int period = 0);
	static void addPassword(const PasswordInformation &password);
	static void removePassword(const PasswordInformation &password);
	static PasswordsManager* getInstance();
	static QStringList getHosts();
	static QVector<PasswordInformation> getPasswords(const QUrl &url, PasswordTypes types = AnyPassword);
	static PasswordMatch hasPassword(const PasswordInformation &password);
	static bool hasPasswords(const QUrl &url, PasswordTypes types = AnyPassword);

protected:
	explicit PasswordsManager(QObject *parent);

private:
	static PasswordsManager *m_instance;
	static PasswordsStorageBackend *m_backend;

signals:
	void passwordsModified();
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::PasswordsManager::PasswordTypes)

#endif
