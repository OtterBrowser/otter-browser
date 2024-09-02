/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtGui/QPainter>

namespace Otter
{

StartPageModel::StartPageModel(QObject *parent) : QStandardItemModel(parent),
	m_bookmark(nullptr)
{
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
	m_bookmark = getRootBookmark();

	clear();

	if (m_bookmark && m_bookmark->isFolder())
	{
		for (int i = 0; i < m_bookmark->rowCount(); ++i)
		{
			const BookmarksModel::Bookmark *bookmark(m_bookmark->getChild(i));

			if (!bookmark)
			{
				continue;
			}

			const BookmarksModel::BookmarkType type(bookmark->getType());
			///TODO feed tile with information about unread? menu for folders / folderlike?

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
				item->setData(true, IsEmptyRole);
			}

			if (url.isValid() && !QFile::exists(getThumbnailPath(identifier)))
			{
				requestThumbnail(url, identifier);
			}

			appendRow(item);
		}
	}

	if (SettingsManager::getOption(SettingsManager::StartPage_ShowAddTileOption).toBool())
	{
		QStandardItem *item(new QStandardItem());
		item->setData(QLatin1String("add"), Qt::AccessibleDescriptionRole);
		item->setDragEnabled(false);
		item->setDropEnabled(false);
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		appendRow(item);
	}

	emit modelModified();
}

void StartPageModel::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
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

	emit modelModified();
}

void StartPageModel::handleBookmarkModified(BookmarksModel::Bookmark *bookmark)
{
	if (!m_bookmark)
	{
		m_bookmark = getRootBookmark();
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
		m_bookmark = getRootBookmark();
	}

	if (!m_bookmark)
	{
		return;
	}

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

void StartPageModel::handleBookmarkRemoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent)
{
	if (m_bookmark && (bookmark == m_bookmark || previousParent == m_bookmark || m_bookmark->isAncestorOf(previousParent)))
	{
		const QString path(getThumbnailPath(bookmark->getIdentifier()));

		if (QFile::exists(path))
		{
			QFile::remove(path);
		}

		QTimer::singleShot(100, this, &StartPageModel::reloadModel);
	}
}

void StartPageModel::handleThumbnailCreated(quint64 identifier, const QPixmap &thumbnail, const QString &title)
{
	if (!m_tileReloads.contains(identifier))
	{
		return;
	}

	const bool needsTitleUpdate(m_tileReloads[identifier]);
	BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(identifier));

	m_tileReloads.remove(identifier);

	if (!bookmark && SessionsManager::isReadOnly() && !thumbnail.isNull())
	{
		Utils::ensureDirectoryExists(SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")));

		thumbnail.save(getThumbnailPath(identifier), "png");
	}

	if (bookmark)
	{
		if (needsTitleUpdate)
		{
			bookmark->setData(title, BookmarksModel::TitleRole);
		}

		const QModelIndex bookmarkIndex(bookmark->index());

		emit isReloadingTileChanged(index(bookmarkIndex.row(), bookmarkIndex.column()));
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
		const QModelIndex index(indexes.at(i));

		if (index.isValid() && static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) == BookmarksModel::UrlBookmark)
		{
			const QVariant data(index.data(BookmarksModel::UrlRole));

			texts.append(data.toString());
			urls.append(data.toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	connect(mimeData, &QMimeData::destroyed, this, &StartPageModel::handleDragEnded);

	return mimeData;
}

BookmarksModel::Bookmark *StartPageModel::getRootBookmark() const
{
	return BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString());
}

BookmarksModel::Bookmark* StartPageModel::getBookmark(const QModelIndex &index)
{
	const QVariant data(index.data(BookmarksModel::IdentifierRole));

	return (data.isValid() ? BookmarksManager::getModel()->getBookmark(data.toULongLong()) : nullptr);
}

QString StartPageModel::getThumbnailPath(quint64 identifier)
{
	return SessionsManager::getWritableDataPath(QLatin1String("thumbnails/")) + QString::number(identifier) + QLatin1String(".png");
}

QVariant StartPageModel::data(const QModelIndex &index, int role) const
{
	if (role == IsReloadingRole)
	{
		return m_tileReloads.contains(index.data(BookmarksModel::IdentifierRole).toULongLong());
	}

	if ((role == Qt::ToolTipRole || role == Qt::StatusTipRole) && index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("add"))
	{
		return tr("Add Tile…");
	}

	return QStandardItemModel::data(index, role);
}

QModelIndex StartPageModel::addTile(const QUrl &url)
{
	if (url.isEmpty())
	{
		return {};
	}

	if (!m_bookmark)
	{
		disconnect(BookmarksManager::getModel(), &BookmarksModel::bookmarkAdded, this, &StartPageModel::handleBookmarkModified);
		disconnect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &StartPageModel::handleBookmarkModified);

		m_bookmark = BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString(), true);

		connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkAdded, this, &StartPageModel::handleBookmarkModified);
		connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &StartPageModel::handleBookmarkModified);
	}

	BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->addBookmark(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, url}}, m_bookmark));

	if (bookmark)
	{
		bookmark = getBookmark(bookmark->index());
	}

	if (!bookmark)
	{
		return {};
	}

	reloadTile(bookmark->index(), true);

	return bookmark->index();
}

QStringList StartPageModel::mimeTypes() const
{
	return {QLatin1String("text/uri-list")};
}

bool StartPageModel::requestThumbnail(const QUrl &url, quint64 identifier, bool needsTitleUpdate)
{
	if (SessionsManager::isReadOnly() || SettingsManager::getOption(SettingsManager::StartPage_TileBackgroundModeOption) != QLatin1String("thumbnail"))
	{
		return false;
	}

	WebPageThumbnailJob *job(AddonsManager::getWebBackend()->createPageThumbnailJob(url, {SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt(), SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt()}));

	if (!job)
	{
		return false;
	}

	connect(job, &WebPageThumbnailJob::jobFinished, this, [=]()
	{
		handleThumbnailCreated(identifier, job->getThumbnail(), job->getTitle());
	});

	m_tileReloads[identifier] = needsTitleUpdate;

	job->start();

	return true;
}

bool StartPageModel::reloadTile(const QModelIndex &index, bool needsTitleUpdate)
{
	if (static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()) != BookmarksModel::UrlBookmark)
	{
		return false;
	}

	const QUrl url(index.data(BookmarksModel::UrlRole).toUrl());

	if (!url.isValid())
	{
		return false;
	}

	const quint64 identifier(index.data(BookmarksModel::IdentifierRole).toULongLong());

	if (url.scheme() != QLatin1String("about"))
	{
		return requestThumbnail(url, identifier, needsTitleUpdate);
	}

	const AddonsManager::SpecialPageInformation information(AddonsManager::getSpecialPage(url.path()));

	if (SessionsManager::isReadOnly())
	{
		handleThumbnailCreated(identifier, {}, information.getTitle());

		return false;
	}

	const QSize size(SettingsManager::getOption(SettingsManager::StartPage_TileWidthOption).toInt(), SettingsManager::getOption(SettingsManager::StartPage_TileHeightOption).toInt());
	QPixmap thumbnail(size);
	thumbnail.fill(Qt::white);

	QPainter painter(&thumbnail);

	information.icon.paint(&painter, {{0, 0}, size});

	m_tileReloads[identifier] = needsTitleUpdate;

	handleThumbnailCreated(identifier, thumbnail, information.getTitle());

	return true;
}

bool StartPageModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action)
	Q_UNUSED(column)

	const QModelIndex index(data->property("x-item-index").toModelIndex());

	if (m_bookmark && index.isValid())
	{
		BookmarksModel::Bookmark *parentBookmark(nullptr);
		BookmarksModel::Bookmark *targetBookmark(BookmarksManager::getModel()->getBookmark(parent));

		if (targetBookmark && targetBookmark->isFolder())
		{
			parentBookmark = targetBookmark;
		}
		else
		{
			parentBookmark = m_bookmark;
			row = (parent.row() + ((index.row() < parent.row()) ? 1 : 0));
		}

		return BookmarksManager::getModel()->moveBookmark(BookmarksManager::getModel()->getBookmark(index), parentBookmark, row);
	}

	if (data->hasUrls())
	{
		const QVector<QUrl> urls(Utils::extractUrls(data));

		for (int i = 0; i < urls.count(); ++i)
		{
			addTile(urls.at(i));
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
