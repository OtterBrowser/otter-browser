/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_FILEPASSWORDSSTORAGEBACKEND_H
#define OTTER_FILEPASSWORDSSTORAGEBACKEND_H

#include "../../../../core/PasswordsStorageBackend.h"

namespace Otter
{

class FilePasswordsStorageBackend final : public PasswordsStorageBackend
{
	Q_OBJECT

public:
	explicit FilePasswordsStorageBackend(QObject *parent = nullptr);

	void clearPasswords(const QString &host) override;
	void clearPasswords(int period = 0) override;
	void addPassword(const PasswordsManager::PasswordInformation &password) override;
	void removePassword(const PasswordsManager::PasswordInformation &password) override;
	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QUrl getHomePage() const override;
	QStringList getHosts() override;
	QVector<PasswordsManager::PasswordInformation> getPasswords(const QUrl &url, PasswordsManager::PasswordTypes types = PasswordsManager::AnyPassword) override;
	PasswordsManager::PasswordMatch hasPassword(const PasswordsManager::PasswordInformation &password) override;
	bool hasPasswords(const QUrl &url, PasswordsManager::PasswordTypes types = PasswordsManager::AnyPassword) override;

protected:
	void ensureInitialized();
	void save();

private:
	QHash<QString, QVector<PasswordsManager::PasswordInformation> > m_passwords;
	bool m_isInitialized;
};

}

#endif
