/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_HISTORYMANAGER_H
#define OTTER_HISTORYMANAGER_H

#include "HistoryModel.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class HistoryManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = nullptr);
	static void clearHistory(uint period = 0);
	static void removeEntry(quint64 identifier);
	static void removeEntries(const QList<quint64> &identifiers);
	static void updateEntry(quint64 identifier, const QUrl &url, const QString &title, const QIcon &icon);
	static HistoryManager* getInstance();
	static HistoryModel* getBrowsingHistoryModel();
	static HistoryModel* getTypedHistoryModel();
	static QIcon getIcon(const QUrl &url);
	static HistoryEntryItem* getEntry(quint64 identifier);
	static QList<HistoryModel::HistoryEntryMatch> findEntries(const QString &prefix);
	static quint64 addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn = false);
	static bool hasEntry(const QUrl &url);

protected:
	explicit HistoryManager(QObject *parent = nullptr);

	void timerEvent(QTimerEvent *event);
	void scheduleSave();

protected slots:
	void optionChanged(int identifier);

private:
	int m_dayTimer;
	int m_saveTimer;

	static HistoryManager *m_instance;
	static HistoryModel *m_browsingHistoryModel;
	static HistoryModel *m_typedHistoryModel;
	static bool m_isEnabled;
	static bool m_isStoringFavicons;

signals:
	void dayChanged();
};

}

#endif
