/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_HISTORYMANAGER_H
#define OTTER_HISTORYMANAGER_H

#include "HistoryModel.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class HistoryManager final : public QObject
{
	Q_OBJECT

public:
	static void createInstance();
	static void clearHistory(uint period = 0);
	static void removeEntry(quint64 identifier);
	static void removeEntries(const QVector<quint64> &identifiers);
	static void updateEntry(quint64 identifier, const QUrl &url, const QString &title, const QIcon &icon);
	static HistoryManager* getInstance();
	static HistoryModel* getBrowsingHistoryModel();
	static HistoryModel* getTypedHistoryModel();
	static QIcon getIcon(const QUrl &url);
	static HistoryModel::Entry* getEntry(quint64 identifier);
	static QVector<HistoryModel::HistoryEntryMatch> findEntries(const QString &prefix, bool isTypedInOnly = false);
	static quint64 addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn = false);
	static bool hasEntry(const QUrl &url);

protected:
	explicit HistoryManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	void scheduleSave();
	void save();

protected slots:
	void handleOptionChanged(int identifier);

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
