/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "BookmarksManager.h"
#include "BookmarksModel.h"
#include "SessionsManager.h"

#include <QtCore/QDateTime>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
BookmarksModel* BookmarksManager::m_model = NULL;

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent),
	 m_saveTimer(0)
{
}

void BookmarksManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (m_model)
		{
			m_model->save(SessionsManager::getProfilePath() + QLatin1String("/bookmarks.xbel"));
		}
	}
}

void BookmarksManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new BookmarksManager(parent);
	}
}

void BookmarksManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}

	emit modelModified();
}

void BookmarksManager::updateVisits(const QUrl &url)
{
	const QUrl adjustedUrl = BookmarksItem::adjustUrl(url);

	if (BookmarksItem::hasBookmark(adjustedUrl))
	{
		const QList<BookmarksItem*> bookmarks = BookmarksItem::getBookmarks(adjustedUrl);

		for (int i = 0; i < bookmarks.count(); ++i)
		{
			bookmarks.at(i)->setData((bookmarks.at(i)->data(BookmarksModel::VisitsRole).toInt() + 1), BookmarksModel::VisitsRole);
			bookmarks.at(i)->setData(QDateTime::currentDateTime(), BookmarksModel::TimeVisitedRole);
		}
	}
}

void BookmarksManager::deleteBookmark(const QUrl &url)
{
	const QUrl adjustedUrl = BookmarksItem::adjustUrl(url);

	if (!hasBookmark(adjustedUrl))
	{
		return;
	}

	const QList<QStandardItem*> items = m_model->findUrls(adjustedUrl);

	for (int i = 0; i < items.count(); ++i)
	{
		items.at(i)->parent()->removeRow(items.at(i)->row());
	}
}

BookmarksManager* BookmarksManager::getInstance()
{
	return m_instance;
}

BookmarksModel* BookmarksManager::getModel()
{
	if (!m_model && m_instance)
	{
		m_model = new BookmarksModel(SessionsManager::getProfilePath() + QLatin1String("/bookmarks.xbel"), m_instance);

		connect(m_model, SIGNAL(itemChanged(QStandardItem*)), m_instance, SLOT(scheduleSave()));
		connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));
		connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), m_instance, SLOT(scheduleSave()));
		connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), m_instance, SLOT(scheduleSave()));

		emit m_instance->modelModified();
	}

	return m_model;
}

BookmarksItem* BookmarksManager::getBookmark(const QString &keyword)
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::getBookmark(keyword);
}

BookmarksItem* BookmarksManager::getBookmark(quint64 identifier)
{
	if (!m_model)
	{
		getModel();
	}

	if (identifier == 0)
	{
		return m_model->getRootItem();
	}

	return BookmarksItem::getBookmark(identifier);
}

QStringList BookmarksManager::getKeywords()
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::getKeywords();
}

QList<QUrl> BookmarksManager::getUrls()
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::getUrls();
}

bool BookmarksManager::hasBookmark(const QUrl &url)
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::hasUrl(url);
}

bool BookmarksManager::hasKeyword(const QString &keyword)
{
	if (!m_model)
	{
		getModel();
	}

	return BookmarksItem::hasKeyword(keyword);
}

}
