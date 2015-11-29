/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_HISTORYMANAGER_H
#define OTTER_HISTORYMANAGER_H

#include "HistoryModel.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtSql/QSqlRecord>

namespace Otter
{

class HistoryManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void clearHistory(int period = 0);
	static HistoryManager* getInstance();
	static HistoryModel* getModel();
	static QStandardItemModel* getTypedHistoryModel();
	static QIcon getIcon(const QUrl &url);
	static HistoryEntryItem* getEntry(quint64 identifier);
	static QList<HistoryModel::HistoryEntryMatch> findEntries(const QString &prefix);
	static qint64 addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn = false);
	static bool hasEntry(const QUrl &url);
	static bool updateEntry(quint64 identifier, const QUrl &url, const QString &title, const QIcon &icon);
	static bool removeEntry(quint64 identifier);
	static bool removeEntries(const QList<quint64> &identifiers);

protected:
	explicit HistoryManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void scheduleCleanup();
	void removeOldEntries(const QDateTime &date = QDateTime());
	static quint64 getRecord(const QLatin1String &table, const QVariantHash &values, bool canCreate = true);
	static quint64 getLocation(const QUrl &url, bool canCreate = true);
	static quint64 getIcon(const QIcon &icon, bool canCreate = true);

protected slots:
	void optionChanged(const QString &option);
	void updateTypedHistoryModel();

private:
	int m_cleanupTimer;
	int m_dayTimer;

	static HistoryManager *m_instance;
	static HistoryModel *m_model;
	static QStandardItemModel *m_typedHistoryModel;
	static bool m_isEnabled;
	static bool m_isStoringFavicons;

signals:
	void cleared();
	void entryAdded(quint64 entry);
	void entryUpdated(quint64 entry);
	void entryRemoved(quint64 entry);
	void dayChanged();
	void typedHistoryModelModified();
};

}

#endif
