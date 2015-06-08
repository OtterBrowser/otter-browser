/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_CONTENTBLOCKINGMANAGER_H
#define OTTER_CONTENTBLOCKINGMANAGER_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

class ContentBlockingProfile;
struct ContentBlockingInformation;

class ContentBlockingManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static ContentBlockingManager* getInstance();
	static QByteArray getStyleSheet(const QVector<int> &profiles);
	static QStringList createSubdomainList(const QString &domain);
	static QVector<ContentBlockingInformation> getProfiles();
	static QMultiHash<QString, QString> getStyleSheetBlackList(const QVector<int> &profiles);
	static QMultiHash<QString, QString> getStyleSheetWhiteList(const QVector<int> &profiles);
	static QVector<int> getProfileList(const QStringList &names);
	static bool updateProfile(const QString &profile);
	static bool isUrlBlocked(const QVector<int> &profiles, const QNetworkRequest &request, const QUrl &baseUrl);

protected:
	explicit ContentBlockingManager(QObject *parent = NULL);

	static void loadProfiles();

private:
	static ContentBlockingManager *m_instance;
	static QVector<ContentBlockingProfile*> m_profiles;
};

}

#endif
