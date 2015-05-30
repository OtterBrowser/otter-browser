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

#include "StartPageModel.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/SettingsManager.h"

#include <QtCore/QMimeData>

namespace Otter
{

StartPageModel::StartPageModel(QObject *parent) : QStandardItemModel(parent),
	m_bookmark(NULL)
{
	reload();

	connect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkMoved(BookmarksItem*,BookmarksItem*,int)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkTrashed(BookmarksItem*)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRestored(BookmarksItem*)), this, SLOT(reload()));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*)), this, SLOT(reload()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void StartPageModel::optionChanged(const QString &option)
{
	if (option == QLatin1String("StartPage/BookmarksFolder"))
	{
		reload();
	}
}

void StartPageModel::reload()
{
	const QString path = SettingsManager::getValue(QLatin1String("StartPage/BookmarksFolder")).toString();

	m_bookmark = BookmarksManager::getModel()->getItem(path);

	if (!m_bookmark)
	{
		const QStringList directories = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

		m_bookmark = BookmarksManager::getModel()->getRootItem();

		for (int i = 0; i < directories.count(); ++i)
		{
			bool found = false;

			for (int j = 0; j < m_bookmark->rowCount(); ++j)
			{
				if (m_bookmark->child(j) && m_bookmark->child(j)->data(Qt::DisplayRole) == directories.at(i))
				{
					m_bookmark = dynamic_cast<BookmarksItem*>(m_bookmark->child(j));

					found = true;

					break;
				}
			}

			if (!found)
			{
				m_bookmark = BookmarksManager::getModel()->addBookmark(BookmarksModel::FolderBookmark, 0, QUrl(), directories.at(i), m_bookmark);
			}
		}
	}

	clear();

	if (m_bookmark)
	{
		for (int i = 0; i < m_bookmark->rowCount(); ++i)
		{
			if (m_bookmark->child(i) && static_cast<BookmarksModel::BookmarkType>(m_bookmark->child(i)->data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
			{
				QStandardItem *item = m_bookmark->child(i)->clone();
				item->setData(m_bookmark->child(i)->data(BookmarksModel::IdentifierRole), BookmarksModel::IdentifierRole);

				appendRow(item);
			}
		}
	}

	emit reloaded();
}

QMimeData* StartPageModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QStringList texts;
	QList<QUrl> urls;

	if (indexes.count() == 1)
	{
		mimeData->setProperty("x-item-index", indexes.at(0));
	}

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (indexes.at(i).isValid() && static_cast<BookmarksModel::BookmarkType>(indexes.at(i).data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
		{
			texts.append(indexes.at(i).data(BookmarksModel::UrlRole).toString());
			urls.append(indexes.at(i).data(BookmarksModel::UrlRole).toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	return mimeData;
}

QStringList StartPageModel::mimeTypes() const
{
	return QStringList(QLatin1String("text/uri-list"));
}

bool StartPageModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action)

	const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt());
	const QModelIndex index = data->property("x-item-index").toModelIndex();

	if (index.isValid())
	{
		if (type == BookmarksModel::FolderBookmark || type == BookmarksModel::RootBookmark || type == BookmarksModel::TrashBookmark)
		{
			return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index.data(BookmarksModel::IdentifierRole).toULongLong()), BookmarksManager::getModel()->getBookmark(parent.data(BookmarksModel::IdentifierRole).toULongLong()), row);
		}

		return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index.data(BookmarksModel::IdentifierRole).toULongLong()), m_bookmark, (parent.row() + ((index.row() < parent.row()) ? 1 : 0)));
	}

	return false;
}

}
