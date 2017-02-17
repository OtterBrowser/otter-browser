/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_HISTORYMODEL_H
#define OTTER_HISTORYMODEL_H

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class HistoryEntryItem : public QStandardItem
{
public:
	void setData(const QVariant &value, int role) override;
	void setItemData(const QVariant &value, int role);

protected:
	explicit HistoryEntryItem();

friend class HistoryModel;
};

class HistoryModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum HistoryEntryRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		IdentifierRole = Qt::UserRole,
		TimeVisitedRole = (Qt::UserRole + 1)
	};

	enum HistoryType
	{
		BrowsingHistory = 0,
		TypedHistory
	};

	struct HistoryEntryMatch
	{
		HistoryEntryItem *entry = nullptr;
		QString match;
		bool isTypedIn = false;
	};

	explicit HistoryModel(const QString &path, HistoryType type, QObject *parent = nullptr);

	void clearExcessEntries(int limit);
	void clearRecentEntries(uint period);
	void clearOldestEntries(int period);
	void removeEntry(quint64 identifier);
	HistoryEntryItem* addEntry(const QUrl &url, const QString &title, const QIcon &icon, const QDateTime &date = QDateTime::currentDateTime(), quint64 identifier = 0);
	HistoryEntryItem* getEntry(quint64 identifier) const;
	QList<HistoryEntryMatch> findEntries(const QString &prefix, bool markAsTypedIn = false) const;
	HistoryType getType() const;
	bool hasEntry(const QUrl &url) const;
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
	QHash<QUrl, QList<HistoryEntryItem*> > m_urls;
	QMap<quint64, HistoryEntryItem*> m_identifiers;
	HistoryType m_type;

signals:
	void cleared();
	void entryAdded(HistoryEntryItem *entry);
	void entryModified(HistoryEntryItem *entry);
	void entryRemoved(HistoryEntryItem *entry);
	void modelModified();
};

}

#endif
