/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

class ContentBlockingList;

class ContentBlockingManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void updateLists();
	static QList<ContentBlockingList*> getBlockingDefinitions();
	static QByteArray getStyleSheetHidingRules();
	static ContentBlockingManager* getInstance();
	static QStringList createSubdomainList(const QString domain);
	static QMultiHash<QString, QString> getSpecificDomainHidingRules();
	static QMultiHash<QString, QString> getHidingRulesExceptions();
	static bool isUrlBlocked(const QNetworkRequest &request);
	static bool isContentBlockingEnabled();

protected:
	explicit ContentBlockingManager(QObject *parent = NULL);

	static void loadLists();

protected slots:
	void updateCustomStyleSheets();

private:
	static ContentBlockingManager *m_instance;
	static QList<ContentBlockingList*> m_blockingLists;
	static QByteArray m_hidingRules;
	static QMultiHash<QString, QString> m_specificDomainHidingRules;
	static QMultiHash<QString, QString> m_hidingRulesExceptions;
	static bool m_isContentBlockingEnabled;

signals:
	void styleSheetsUpdated();
};

}

#endif
