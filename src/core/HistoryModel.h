/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

class HistoryModel final : public QStandardItemModel
{
	Q_OBJECT

public:
	enum HistoryEntryRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		IdentifierRole = Qt::UserRole,
		TimeVisitedRole
	};

	enum HistoryType
	{
		BrowsingHistory = 0,
		TypedHistory
	};

	class Entry final : public QStandardItem
	{
	public:
		void setData(const QVariant &value, int role) override;
		void setItemData(const QVariant &value, int role);
		QString getTitle() const;
		QUrl getUrl() const;
		QDateTime getTimeVisited() const;
		QIcon getIcon() const;
		quint64 getIdentifier() const;
		bool isValid() const;

	protected:
		explicit Entry();

	friend class HistoryModel;
	};

	struct HistoryEntryMatch final
	{
		Entry *entry = nullptr;
		QString match;
		bool isTypedIn = false;
	};

	explicit HistoryModel(const QString &path, HistoryType type, QObject *parent = nullptr);

	void clearExcessEntries(int limit);
	void clearRecentEntries(uint period);
	void clearOldestEntries(int period);
	void removeEntry(quint64 identifier);
	Entry* addEntry(const QUrl &url, const QString &title, const QIcon &icon, const QDateTime &date = QDateTime::currentDateTimeUtc(), quint64 identifier = 0);
	Entry* getEntry(quint64 identifier) const;
	QDateTime getLastVisitTime(const QUrl &url) const;
	QVector<HistoryEntryMatch> findEntries(const QString &prefix, bool markAsTypedIn = false) const;
	HistoryType getType() const;
	bool hasEntry(const QUrl &url) const;
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
	QHash<QUrl, QVector<Entry*> > m_urls;
	QMap<quint64, Entry*> m_identifiers;
	HistoryType m_type;

signals:
	void cleared();
	void entryAdded(Entry *entry);
	void entryModified(Entry *entry);
	void entryRemoved(Entry *entry);
	void modelModified();
};

}

#endif
