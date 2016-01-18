/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	void setData(const QVariant &value, int role);
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
		TimeVisitedRole = (Qt::UserRole + 1),
		TypedInRole = (Qt::UserRole + 2)
	};

	struct HistoryEntryMatch
	{
		HistoryEntryItem *entry;
		QString match;

		HistoryEntryMatch () : entry(NULL) {}
	};

	explicit HistoryModel(const QString &path, QObject *parent = NULL);

	void clearEntries(uint period);
	void removeEntry(quint64 identifier);
	HistoryEntryItem* addEntry(const QUrl &url, const QString &title, const QIcon &icon, const QDateTime &date = QDateTime::currentDateTime(), quint64 identifier = 0);
	HistoryEntryItem* getEntry(quint64 identifier) const;
	QList<HistoryEntryMatch> findEntries(const QString &prefix) const;
	bool hasEntry(const QUrl &url) const;
	bool save(const QString &path) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);

private:
	QHash<QUrl, QList<HistoryEntryItem*> > m_urls;
	QMap<quint64, HistoryEntryItem*> m_identifiers;

signals:
	void entryAdded(HistoryEntryItem *entry);
	void entryModified(HistoryEntryItem *entry);
	void entryRemoved(HistoryEntryItem *entry);
	void modelModified();
};

}

#endif
