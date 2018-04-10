/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkAdded, this, &StartPageModel::handleBookmarkModified);
	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &StartPageModel::handleBookmarkModified);
	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkRestored, this, &StartPageModel::handleBookmarkModified);
	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkMoved, this, &StartPageModel::handleBookmarkMoved);
	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkTrashed, this, &StartPageModel::handleBookmarkMoved);
	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkRemoved, this, &StartPageModel::handleBookmarkRemoved);
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &StartPageModel::handleOptionChanged);
}

void StartPageModel::reloadModel()
{
	m_bookmark = BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());

	clear();

	if (m_bookmark)
	{
		for (int i = 0; i < m_bookmark->rowCount(); ++i)
		{
			const BookmarksModel::Bookmark *bookmark(static_cast<BookmarksModel::Bookmark*>(m_bookmark->child(i)));

			if (bookmark)
			{
				const BookmarksModel::BookmarkType type(bookmark->getType());

				if (type != BookmarksModel::UrlBookmark && type != BookmarksModel::FolderBookmark)
				{
					continue;
				}

				const quint64 identifier(bookmark->getIdentifier());
				const QUrl url(bookmark->getUrl());
				QStandardItem *item(bookmark->clone());
				item->setData(identifier, BookmarksModel::IdentifierRole);
				item->setData(bookmark->getTitle(), Qt::ToolTipRole);
				item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

				if (type == BookmarksModel::FolderBookmark && bookmark->rowCount() == 0)
				{
					item->setEnabled(false);
				}
				else if (url.isValid() && SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail") && !QFile::exists(getThumbnailPath(identifier)))
				{
					ThumbnailRequestInformation thumbnailRequestInformation;
					thumbnailRequestInformation.bookmarkIdentifier = identifier;

					m_reloads[url] = thumbnailRequestInformation;

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
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		appendRow(item);
	}

	emit modelModified();
}

void StartPageModel::addTile(const QUrl &url)
{
	if (!m_bookmark)
	{
		disconnect(BookmarksManager::getModel(), &BookmarksModel::bookmarkAdded, this, &StartPageModel::handleBookmarkModified);
		disconnect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &StartPageModel::handleBookmarkModified);

		const QStringList directories(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString().split(QLatin1Char('/'), QString::SkipEmptyParts));

		m_bookmark = BookmarksManager::getModel()->getRootItem();

		for (int i = 0; i < directories.count(); ++i)
		{
			bool hasMatch(false);

			for (int j = 0; j < m_bookmark->rowCount(); ++j)
			{
				BookmarksModel::Bookmark *childBookmark(static_cast<BookmarksModel::Bookmark*>(m_bookmark->child(j)));

				if (childBookmark && childBookmark->getRawData(Qt::DisplayRole).toString() == directories.at(i))
				{
					m_bookmark = childBookmark;

					hasMatch = true;

					break;
				}
			}

			if (!hasMatch)
			{
				m_bookmark = BookmarksManager::getModel()->addBookmark(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, directories.at(i)}}, m_bookmark);
			}
		}

		connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkAdded, this, &StartPageModel::handleBookmarkModified);
		connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &StartPageModel::handleBookmarkModified);
	}

	const BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->addBookmark(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, url}}, m_bookmark));

	if (bookmark)
	{
		reloadTile(bookmark->index(), true);
	}
}

void StartPageModel::reloadTile(const QModelIndex &index, bool needsTitleUpdate)
{
	const QUrl url(index.data(BookmarksModel::UrlRole).toUrl());

	if (!url.isValid())
	{
		return;
	}

	QSize size;
	ThumbnailRequestInformation thumbnailRequestInformation;
	thumbnailRequestInformation.bookmarkIdentifier = index.data(BookmarksModel::IdentifierRole).toULongLong();
	thumbnailRequestInformation.needsTitleUpdate = needsTitleUpdate;

	if (!SessionsManager::isReadOnly() && SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) == QLatin1String("thumbnail"))
	{
		size = QSize(SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt(), SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt());
	}
	else if (!needsTitleUpdate)
	{
		return;
	}

	if (url.scheme() == QLatin1String("about"))
	{
		const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(url.path()));

		if (SessionsManager::isReadOnly())
		{
			handleThumbnailCreated(url, {}, information.getTitle());
		}
		else
		{
			QPixmap thumbnail(size);
			thumbnail.fill(Qt::white);

			QPainter painter(&thumbnail);

			information.icon.paint(&painter, QRect(QPoint(0, 0), size));

			m_reloads[index.data(BookmarksModel::UrlRole).toUrl()] = thumbnailRequestInformation;

			handleThumbnailCreated(url, thumbnail, information.getTitle());
		}
	}
	else if (AddonsManager::getWebBackend()->requestThumbnail(url, size))
	{
		m_reloads[index.data(BookmarksModel::UrlRole).toUrl()] = thumbnailRequestInformation;
	}
}

void StartPageModel::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Backends_WebOption:
			connect(AddonsManager::getWebBackend(), &WebBackend::thumbnailAvailable, this, &StartPageModel::handleThumbnailCreated);

			break;
		case SettingsManager::StartPage_BookmarksFolderOption:
		case SettingsManager::StartPage_ShowAddTileOption:
			reloadModel();

			break;
		default:
			break;
	}
}

void StartPageModel::handleDragEnded()
{
	for (int i = 0; i < rowCount(); ++i)
	{
		const QModelIndex index(this->index(i, 0));

		if (index.data(IsDraggedRole).toBool())
		{
			setData(index, {}, IsDraggedRole);

			emit isReloadingTileChanged(index);
		}
	}
}

void StartPageModel::handleBookmarkModified(BookmarksModel::Bookmark *bookmark)
{
	if (!m_bookmark)
	{
		m_bookmark = BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());
	}

	if (m_bookmark && (bookmark == m_bookmark || m_bookmark->isAncestorOf(bookmark)))
	{
		reloadModel();
	}
}

void StartPageModel::handleBookmarkMoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent)
{
	if (!m_bookmark)
	{
		m_bookmark = BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());
	}

	if (m_bookmark)
	{
		if (bookmark->parent() != m_bookmark)
		{
			const QString path(getThumbnailPath(bookmark->getIdentifier()));

			if (QFile::exists(path))
			{
				QFile::remove(path);
			}
		}

		if (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(bookmark) || m_bookmark->isAncestorOf(previousParent))
		{
			reloadModel();
		}
	}
}

void StartPageModel::handleBookmarkRemoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent)
{
	if (m_bookmark && (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(previousParent)))
	{
		const QString path(getThumbnailPath(bookmark->getIdentifier()));

		if (QFile::exists(path))
		{
			QFile::remove(path);
		}

		reloadModel();
	}
}

void StartPageModel::handleThumbnailCreated(const QUrl &url, const QPixmap &thumbnail, const QString &title)
{
	if (!m_reloads.contains(url))
	{
		return;
	}

	const ThumbnailRequestInformation information(m_reloads[url]);
	BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(information.bookmarkIdentifier));

	m_reloads.remove(url);

	if (!SessionsManager::isReadOnly() && !thumbnail.isNull() && bookmark)
	{
		QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")));

		thumbnail.save(getThumbnailPath(information.bookmarkIdentifier), "png");
	}

	if (bookmark)
	{
		if (information.needsTitleUpdate)
		{
			bookmark->setData(title, BookmarksModel::TitleRole);
		}

		emit isReloadingTileChanged(index(bookmark->index().row(), bookmark->index().column()));
	}
}

QMimeData* StartPageModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData(new QMimeData());
	QStringList texts;
	texts.reserve(indexes.count());

	QList<QUrl> urls;
	urls.reserve(indexes.count());

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

	connect(mimeData, &QMimeData::destroyed, this, &StartPageModel::handleDragEnded);

	return mimeData;
}

QString StartPageModel::getThumbnailPath(quint64 identifier)
{
	return SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(identifier) + QLatin1String(".png");
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
	return {QLatin1String("text/uri-list")};
}

bool StartPageModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action)
	Q_UNUSED(column)

	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt()));
	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (m_bookmark)
	{
		if (index.isValid())
		{
			if (type == BookmarksModel::FolderBookmark || type == BookmarksModel::RootBookmark || type == BookmarksModel::TrashBookmark)
			{
				return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index), BookmarksManager::getModel()->getBookmark(parent), row);
			}

			return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index), m_bookmark, (parent.row() + ((index.row() < parent.row()) ? 1 : 0)));
		}
		else if (data->hasUrls())
		{
			const QVector<QUrl> urls(Utils::extractUrls(data));

			for (int i = 0; i < urls.count(); ++i)
			{
				addTile(urls.at(i));
			}
		}
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
