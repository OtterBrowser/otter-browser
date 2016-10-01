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

#ifndef OTTER_FILEPASSWORDSSTORAGEBACKEND_H
#define OTTER_FILEPASSWORDSSTORAGEBACKEND_H

#include "../../../../core/PasswordsStorageBackend.h"

namespace Otter
{

class FilePasswordsStorageBackend : public PasswordsStorageBackend
{
	Q_OBJECT

public:
	explicit FilePasswordsStorageBackend(QObject *parent = NULL);

	void clearPasswords(const QString &host = QString());
	void addPassword(const PasswordsManager::PasswordInformation &password);
	void removePassword(const PasswordsManager::PasswordInformation &password);
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QUrl getHomePage() const;
	QIcon getIcon() const;
	QStringList getHosts();
	QList<PasswordsManager::PasswordInformation> getPasswords(const QUrl &url, PasswordsManager::PasswordTypes types = PasswordsManager::AnyPassword);
	PasswordsManager::PasswordMatch hasPassword(const PasswordsManager::PasswordInformation &password);
	bool hasPasswords(const QUrl &url, PasswordsManager::PasswordTypes types = PasswordsManager::AnyPassword);

protected:
	void initialize();
	void save();

private:
	QHash<QString, QList<PasswordsManager::PasswordInformation> > m_passwords;
	bool m_isInitialized;
};

}

#endif
