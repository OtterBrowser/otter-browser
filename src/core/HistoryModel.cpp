/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

#include "HistoryModel.h"
#include "Utils.h"

namespace Otter
{

HistoryEntryItem::HistoryEntryItem() : QStandardItem()
{
}

void HistoryEntryItem::setData(const QVariant &value, int role)
{
	if (model() && qobject_cast<HistoryModel*>(model()))
	{
		model()->setData(index(), value, role);
	}
	else
	{
		QStandardItem::setData(value, role);
	}
}

void HistoryEntryItem::setItemData(const QVariant &value, int role)
{
	QStandardItem::setData(value, role);
}

HistoryModel::HistoryModel(QObject *parent) : QStandardItemModel(parent)
{
}

void HistoryModel::removeEntry(quint64 identifier)
{
	HistoryEntryItem *entry = getEntry(identifier);

	if (!entry)
	{
		return;
	}

	const QUrl url = Utils::normalizeUrl(entry->data(UrlRole).toUrl());

	if (m_urls.contains(url))
	{
		m_urls[url].removeAll(entry);

		if (m_urls[url].isEmpty())
		{
			m_urls.remove(url);
		}
	}

	if (identifier > 0 && m_identifiers.contains(identifier))
	{
		m_identifiers.remove(identifier);
	}

	emit entryRemoved(entry);

	removeRow(entry->row());

	emit modelModified();
}

HistoryEntryItem* HistoryModel::addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn, const QDateTime &date, quint64 identifier)
{
	HistoryEntryItem *entry = new HistoryEntryItem();
	entry->setIcon(icon);

	appendRow(entry);
	setData(entry->index(), url, UrlRole);
	setData(entry->index(), title, TitleRole);
	setData(entry->index(), date, TimeVisitedRole);
	setData(entry->index(), isTypedIn, TypedInRole);

	if (identifier == 0 || m_identifiers.contains(identifier))
	{
		identifier = (m_identifiers.isEmpty() ? 1 : (m_identifiers.keys().last() + 1));
	}

	setData(entry->index(), identifier, IdentifierRole);

	m_identifiers[identifier] = entry;

	return entry;
}

HistoryEntryItem* HistoryModel::getEntry(quint64 identifier) const
{
	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	return NULL;
}

QList<HistoryModel::HistoryEntryMatch> HistoryModel::findEntries(const QString &prefix) const
{
	QList<HistoryEntryItem*> matchedEntries;
	QList<HistoryModel::HistoryEntryMatch> allMatches;
	QList<HistoryModel::HistoryEntryMatch> currentMatches;
	QMultiMap<QDateTime, HistoryModel::HistoryEntryMatch> matchesMap;
	QHash<QUrl, QList<HistoryEntryItem*> >::const_iterator urlsIterator;

	for (urlsIterator = m_urls.constBegin(); urlsIterator != m_urls.constEnd(); ++urlsIterator)
	{
		if (urlsIterator.value().isEmpty() || matchedEntries.contains(urlsIterator.value().first()))
		{
			continue;
		}

		const QString result = Utils::matchUrl(urlsIterator.key(), prefix);

		if (!result.isEmpty())
		{
			HistoryEntryMatch match;
			match.entry = urlsIterator.value().first();
			match.match = result;

			matchesMap.insert(match.entry->data(TimeVisitedRole).toDateTime(), match);

			matchedEntries.append(match.entry);
		}
	}

	currentMatches = matchesMap.values();

	matchesMap.clear();

	for (int i = (currentMatches.count() - 1); i >= 0; --i)
	{
		allMatches.append(currentMatches.at(i));
	}

	return allMatches;
}

bool HistoryModel::hasEntry(const QUrl &url) const
{
	return m_urls.contains(url);
}

bool HistoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	HistoryEntryItem *entry = dynamic_cast<HistoryEntryItem*>(itemFromIndex(index));

	if (!entry)
	{
		return QStandardItemModel::setData(index, value, role);
	}

	if (role == UrlRole && value.toUrl() != index.data(UrlRole).toUrl())
	{
		const QUrl oldUrl = Utils::normalizeUrl(index.data(UrlRole).toUrl());
		const QUrl newUrl = Utils::normalizeUrl(value.toUrl());

		if (!oldUrl.isEmpty() && m_urls.contains(oldUrl))
		{
			m_urls[oldUrl].removeAll(entry);

			if (m_urls[oldUrl].isEmpty())
			{
				m_urls.remove(oldUrl);
			}
		}

		if (!newUrl.isEmpty())
		{
			if (!m_urls.contains(newUrl))
			{
				m_urls[newUrl] = QList<HistoryEntryItem*>();
			}

			m_urls[newUrl].append(entry);
		}
	}

	entry->setItemData(value, role);

	switch (role)
	{
		case TitleRole:
		case UrlRole:
		case IdentifierRole:
		case TimeVisitedRole:
			emit entryModified(entry);
			emit modelModified();

			break;
	}

	return true;
}

}
