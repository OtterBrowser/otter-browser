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

#include "StartPageModel.h"
#include "../../../core/AddonsManager.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/BookmarksModel.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/WebBackend.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMimeData>

namespace Otter
{

StartPageModel::StartPageModel(QObject *parent) : QStandardItemModel(parent),
	m_bookmark(NULL)
{
	optionChanged(SettingsManager::Backends_WebOption);
	reloadModel();

	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(reloadModel()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int)));
}

void StartPageModel::optionChanged(int identifier)
{
	if (identifier == SettingsManager::Backends_WebOption)
	{
		connect(AddonsManager::getWebBackend(), SIGNAL(thumbnailAvailable(QUrl,QPixmap,QString)), this, SLOT(thumbnailCreated(QUrl,QPixmap,QString)));
	}
	else if (identifier == SettingsManager::StartPage_BookmarksFolderOption || identifier == SettingsManager::StartPage_ShowAddTileOption)
	{
		reloadModel();
	}
}

void StartPageModel::dragEnded()
{
	for (int i = 0; i < rowCount(); ++i)
	{
		if (item(i) && item(i)->data(BookmarksModel::UserRole).toBool())
		{
			item(i)->setData(QVariant(), BookmarksModel::UserRole);

			emit isReloadingTileChanged(item(i)->index());
		}
	}
}

void StartPageModel::thumbnailCreated(const QUrl &url, const QPixmap &thumbnail, const QString &title)
{
	if (!m_reloads.contains(url))
	{
		return;
	}

	if (!thumbnail.isNull())
	{
		const QString path(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")));

		QDir().mkpath(path);

		thumbnail.save(path + QString::number(m_reloads[url].first) + QLatin1String(".png"), "png");
	}

	BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(m_reloads[url].first));

	if (bookmark)
	{
		if (m_reloads[url].second)
		{
			bookmark->setData(title, BookmarksModel::TitleRole);
		}

		emit isReloadingTileChanged(index(bookmark->index().row(), bookmark->index().column()));
	}

	m_reloads.remove(url);
}

void StartPageModel::reloadModel()
{
	const QString path(SettingsManager::getValue(SettingsManager::StartPage_BookmarksFolderOption).toString());

	m_bookmark = BookmarksManager::getModel()->getItem(path);

	if (!m_bookmark)
	{
		const QStringList directories(path.split(QLatin1Char('/'), QString::SkipEmptyParts));

		m_bookmark = BookmarksManager::getModel()->getRootItem();

		for (int i = 0; i < directories.count(); ++i)
		{
			bool hasFound(false);

			for (int j = 0; j < m_bookmark->rowCount(); ++j)
			{
				if (m_bookmark->child(j) && m_bookmark->child(j)->data(Qt::DisplayRole) == directories.at(i))
				{
					m_bookmark = dynamic_cast<BookmarksItem*>(m_bookmark->child(j));

					hasFound = true;

					break;
				}
			}

			if (!hasFound)
			{
				disconnect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(reloadModel()));

				m_bookmark = BookmarksManager::getModel()->addBookmark(BookmarksModel::FolderBookmark, 0, QUrl(), directories.at(i), m_bookmark);

				connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(reloadModel()));
			}
		}
	}

	clear();

	if (m_bookmark)
	{
		for (int i = 0; i < m_bookmark->rowCount(); ++i)
		{
			if (m_bookmark->child(i))
			{
				const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(m_bookmark->child(i)->data(BookmarksModel::TypeRole).toInt()));

				if (type != BookmarksModel::UrlBookmark && type != BookmarksModel::FolderBookmark)
				{
					continue;
				}

				const quint64 identifier(m_bookmark->child(i)->data(BookmarksModel::IdentifierRole).toULongLong());
				const QUrl url(m_bookmark->child(i)->data(BookmarksModel::UrlRole).toUrl());
				QStandardItem *item(m_bookmark->child(i)->clone());
				item->setData(identifier, BookmarksModel::IdentifierRole);

				if (url.isValid() && SettingsManager::getValue(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail") && !QFile::exists(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(identifier) + QLatin1String(".png")))
				{
					m_reloads[url] = qMakePair(identifier, false);

					AddonsManager::getWebBackend()->requestThumbnail(url, QSize(SettingsManager::getValue(SettingsManager::StartPage_TileWidthOption).toInt(), SettingsManager::getValue(SettingsManager::StartPage_TileHeightOption).toInt()));
				}

				appendRow(item);
			}
		}
	}

	if (SettingsManager::getValue(SettingsManager::StartPage_ShowAddTileOption).toBool())
	{
		QStandardItem *item(new QStandardItem());
		item->setData(tr("Add Tile…"), Qt::ToolTipRole);
		item->setData(tr("Add Tile…"), Qt::StatusTipRole);
		item->setData(QLatin1String("add"), Qt::AccessibleDescriptionRole);
		item->setDragEnabled(false);
		item->setDropEnabled(false);

		appendRow(item);
	}

	emit modelModified();
}

void StartPageModel::reloadTile(const QModelIndex &index, bool full)
{
	const QUrl url(index.data(BookmarksModel::UrlRole).toUrl());

	if (url.isValid())
	{
		QSize size;

		if (SettingsManager::getValue(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail"))
		{
			size.setWidth(SettingsManager::getValue(SettingsManager::StartPage_TileWidthOption).toInt());
			size.setHeight(SettingsManager::getValue(SettingsManager::StartPage_TileHeightOption).toInt());
		}
		else if (!full)
		{
			return;
		}

		if (AddonsManager::getWebBackend()->requestThumbnail(url, size))
		{
			m_reloads[index.data(BookmarksModel::UrlRole).toUrl()] = qMakePair(index.data(BookmarksModel::IdentifierRole).toULongLong(), full);
		}
	}
}

QMimeData* StartPageModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData(new QMimeData());
	QStringList texts;
	QList<QUrl> urls;

	if (indexes.count() == 1)
	{
		mimeData->setProperty("x-item-index", indexes.at(0));

		itemFromIndex(indexes.at(0))->setData(true, BookmarksModel::UserRole);
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

	connect(mimeData, SIGNAL(destroyed()), this, SLOT(dragEnded()));

	return mimeData;
}

QStringList StartPageModel::mimeTypes() const
{
	return QStringList(QLatin1String("text/uri-list"));
}

bool StartPageModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action)
	Q_UNUSED(column)

	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt()));
	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (index.isValid())
	{
		if (type == BookmarksModel::FolderBookmark || type == BookmarksModel::RootBookmark || type == BookmarksModel::TrashBookmark)
		{
			return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index), BookmarksManager::getModel()->getBookmark(parent), row);
		}

		return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index), m_bookmark, (parent.row() + ((index.row() < parent.row()) ? 1 : 0)));
	}

	return false;
}

bool StartPageModel::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		reloadModel();
	}

	return QStandardItemModel::event(event);
}

bool StartPageModel::isReloadingTile(const QModelIndex &index) const
{
	return m_reloads.contains(index.data(BookmarksModel::UrlRole).toUrl());
}

}
