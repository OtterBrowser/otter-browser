/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StartPageModel.h"
#include "../../../core/AddonsManager.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/WebBackend.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtGui/QPainter>

namespace Otter
{

StartPageModel::StartPageModel(QObject *parent) : QStandardItemModel(parent),
	m_bookmark(nullptr)
{
	handleOptionChanged(SettingsManager::Backends_WebOption);
	reloadModel();

	connect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkModified(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRestored(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkMoved(BookmarksItem*,BookmarksItem*,int)), this, SLOT(handleBookmarkMoved(BookmarksItem*,BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkTrashed(BookmarksItem*,BookmarksItem*)), this, SLOT(handleBookmarkMoved(BookmarksItem*,BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*,BookmarksItem*)), this, SLOT(handleBookmarkRemoved(BookmarksItem*,BookmarksItem*)));
	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int)));
}

void StartPageModel::dragEnded()
{
	for (int i = 0; i < rowCount(); ++i)
	{
		if (item(i) && item(i)->data(IsDraggedRole).toBool())
		{
			item(i)->setData(QVariant(), IsDraggedRole);

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
	m_bookmark = BookmarksManager::getModel()->getItem(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());

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

				if (url.isValid() && SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail") && !QFile::exists(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(identifier) + QLatin1String(".png")))
				{
					m_reloads[url] = {identifier, false};

					AddonsManager::getWebBackend()->requestThumbnail(url, QSize(SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt(), SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt()));
				}

				appendRow(item);
			}
		}
	}

	if (SettingsManager::getOption(SettingsManager::StartPage_ShowAddTileOption).toBool())
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

void StartPageModel::addTile(const QUrl &url)
{
	if (!m_bookmark)
	{
		disconnect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
		disconnect(BookmarksManager::getModel(), SIGNAL(bookmarkModified(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));

		const QStringList directories(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString().split(QLatin1Char('/'), QString::SkipEmptyParts));

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
				m_bookmark = BookmarksManager::getModel()->addBookmark(BookmarksModel::FolderBookmark, 0, QUrl(), directories.at(i), m_bookmark);
			}
		}

		connect(BookmarksManager::getModel(), SIGNAL(bookmarkAdded(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
		connect(BookmarksManager::getModel(), SIGNAL(bookmarkModified(BookmarksItem*)), this, SLOT(handleBookmarkModified(BookmarksItem*)));
	}

	BookmarksItem *bookmark(BookmarksManager::getModel()->addBookmark(BookmarksModel::UrlBookmark, 0, url, QString(), m_bookmark));

	if (bookmark)
	{
		reloadTile(bookmark->index(), true);
	}
}

void StartPageModel::reloadTile(const QModelIndex &index, bool full)
{
	const QUrl url(index.data(BookmarksModel::UrlRole).toUrl());

	if (url.isValid())
	{
		QSize size;

		if (SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail"))
		{
			size.setWidth(SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt());
			size.setHeight(SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt());
		}
		else if (!full)
		{
			return;
		}

		if (url.scheme() == QLatin1String("about"))
		{
			const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(url.path()));

			QPixmap thumbnail(size);
			thumbnail.fill();

			QPainter painter(&thumbnail);

			information.icon.paint(&painter, QRect(QPoint(0, 0), size));

			m_reloads[index.data(BookmarksModel::UrlRole).toUrl()] = {index.data(BookmarksModel::IdentifierRole).toULongLong(), full};

			thumbnailCreated(url, thumbnail, information.getTitle());
		}
		else if (AddonsManager::getWebBackend()->requestThumbnail(url, size))
		{
			m_reloads[index.data(BookmarksModel::UrlRole).toUrl()] = {index.data(BookmarksModel::IdentifierRole).toULongLong(), full};
		}
	}
}

void StartPageModel::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Backends_WebOption:
			connect(AddonsManager::getWebBackend(), SIGNAL(thumbnailAvailable(QUrl,QPixmap,QString)), this, SLOT(thumbnailCreated(QUrl,QPixmap,QString)));

			break;
		case SettingsManager::StartPage_BookmarksFolderOption:
		case SettingsManager::StartPage_ShowAddTileOption:
			reloadModel();

			break;
		default:
			break;
	}
}

void StartPageModel::handleBookmarkModified(BookmarksItem *bookmark)
{
	if (!m_bookmark)
	{
		m_bookmark = BookmarksManager::getModel()->getItem(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());
	}

	if (m_bookmark && (bookmark == m_bookmark || m_bookmark->isAncestorOf(bookmark)))
	{
		reloadModel();
	}
}

void StartPageModel::handleBookmarkMoved(BookmarksItem *bookmark, BookmarksItem *previousParent)
{
	if (!m_bookmark)
	{
		m_bookmark = BookmarksManager::getModel()->getItem(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());
	}

	if (m_bookmark && (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(bookmark) || m_bookmark->isAncestorOf(previousParent)))
	{
		reloadModel();
	}
}

void StartPageModel::handleBookmarkRemoved(BookmarksItem *bookmark, BookmarksItem *previousParent)
{
	if (m_bookmark && (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(previousParent)))
	{
		reloadModel();
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

		itemFromIndex(indexes.at(0))->setData(true, IsDraggedRole);
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

QVariant StartPageModel::data(const QModelIndex &index, int role) const
{
	if (role == IsReloadingRole)
	{
		return m_reloads.contains(index.data(BookmarksModel::UrlRole).toUrl());
	}

	return QStandardItemModel::data(index, role);
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

	if (m_bookmark && index.isValid())
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

}
