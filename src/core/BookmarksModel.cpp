/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksModel.h"
#include "Utils.h"
#include "WebBackend.h"
#include "WebBackendsManager.h"

#include <QtCore/QMimeData>

namespace Otter
{

QHash<QString, QList<BookmarksItem*> > BookmarksItem::m_urls;
QHash<QString, BookmarksItem*> BookmarksItem::m_keywords;

BookmarksItem::BookmarksItem(BookmarkType type, const QUrl &url, const QString &title) : QStandardItem()
{
	setData(type, BookmarksModel::TypeRole);
	setData(url, BookmarksModel::UrlRole);
	setData(title, BookmarksModel::TitleRole);

	if (type == UrlBookmark || type == SeparatorBookmark)
	{
		setDropEnabled(false);
	}

	if (type == RootBookmark)
	{
		setDragEnabled(false);
	}

	if (type == TrashBookmark)
	{
		setEnabled(false);
		setDragEnabled(false);
	}
}

BookmarksItem::~BookmarksItem()
{
	if (!data(BookmarksModel::UrlRole).toString().isEmpty())
	{
		const QString url = data(BookmarksModel::UrlRole).toUrl().toString();

		if (m_urls.contains(url))
		{
			m_urls[url].removeAll(this);

			if (m_urls[url].isEmpty())
			{
				m_urls.remove(url);
			}
		}
	}

	if (!data(BookmarksModel::KeywordRole).toString().isEmpty() && m_keywords.contains(data(BookmarksModel::UrlRole).toString()))
	{
		m_keywords.remove(data(BookmarksModel::UrlRole).toString());
	}
}

void BookmarksItem::setData(const QVariant &value, int role)
{
	if (role == BookmarksModel::UrlRole && value.toUrl() != data(BookmarksModel::UrlRole).toUrl())
	{
		const QString oldUrl = data(BookmarksModel::UrlRole).toUrl().toString();
		const QString newUrl = value.toUrl().toString();

		if (!oldUrl.isEmpty() && m_urls.contains(oldUrl))
		{
			m_urls[oldUrl].removeAll(this);

			if (m_urls[oldUrl].isEmpty())
			{
				m_urls.remove(oldUrl);
			}
		}

		if (!newUrl.isEmpty())
		{
			if (!m_urls.contains(newUrl))
			{
				m_urls[newUrl] = QList<BookmarksItem*>();
			}

			m_urls[newUrl].append(this);
		}
	}
	else if (role == BookmarksModel::KeywordRole && value.toString() != data(BookmarksModel::KeywordRole).toString())
	{
		const QString oldKeyword = data(BookmarksModel::KeywordRole).toString();
		const QString newKeyword = value.toString();

		if (!oldKeyword.isEmpty() && m_keywords.contains(oldKeyword))
		{
			m_keywords.remove(oldKeyword);
		}

		if (!newKeyword.isEmpty())
		{
			m_keywords[newKeyword] = this;
		}
	}

	QStandardItem::setData(value, role);
}

QStandardItem* BookmarksItem::clone() const
{
	BookmarksItem *item = new BookmarksItem(static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt()), data(BookmarksModel::UrlRole).toUrl(), data(BookmarksModel::TitleRole).toString());
	item->setData(data(BookmarksModel::DescriptionRole), BookmarksModel::DescriptionRole);
	item->setData(data(BookmarksModel::KeywordRole), BookmarksModel::KeywordRole);
	item->setData(data(BookmarksModel::TimeAddedRole), BookmarksModel::TimeAddedRole);
	item->setData(data(BookmarksModel::TimeModifiedRole), BookmarksModel::TimeModifiedRole);
	item->setData(data(BookmarksModel::TimeVisitedRole), BookmarksModel::TimeVisitedRole);
	item->setData(data(BookmarksModel::VisitsRole), BookmarksModel::VisitsRole);

	return item;
}

BookmarksItem* BookmarksItem::getBookmark(const QString &keyword)
{
	if (m_keywords.contains(keyword))
	{
		return m_keywords[keyword];
	}

	return NULL;
}

QList<BookmarksItem*> BookmarksItem::getBookmarks(const QString &url)
{
	if (m_urls.contains(url))
	{
		return m_urls[url];
	}

	return QList<BookmarksItem*>();
}

QStringList BookmarksItem::getKeywords()
{
	return m_keywords.keys();
}

QStringList BookmarksItem::getUrls()
{
	return m_urls.keys();
}

QVariant BookmarksItem::data(int role) const
{
	if (role == Qt::DecorationRole)
	{
		const BookmarkType type = static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt());

		if (type == RootBookmark || type == FolderBookmark)
		{
			return Utils::getIcon(QLatin1String("inode-directory"));
		}
		else if (type == TrashBookmark)
		{
			return Utils::getIcon(QLatin1String("user-trash"));
		}
		else if (type == UrlBookmark)
		{
			return WebBackendsManager::getBackend()->getIconForUrl(data(BookmarksModel::UrlRole).toUrl());
		}

		return QVariant();
	}

	if (role == Qt::AccessibleDescriptionRole && static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt()) == SeparatorBookmark)
	{
		return QLatin1String("separator");
	}

	return QStandardItem::data(role);
}

bool BookmarksItem::hasBookmark(const QString &url)
{
	return m_urls.contains(QUrl(url).toString());
}

bool BookmarksItem::hasKeyword(const QString &keyword)
{
	return m_keywords.contains(keyword);
}

bool BookmarksItem::hasUrl(const QString &url)
{
	return m_urls.contains(url);
}

BookmarksModel::BookmarksModel(QObject *parent) : QStandardItemModel(parent)
{
	appendRow(new BookmarksItem(BookmarksItem::RootBookmark, QUrl(), tr("Bookmarks")));
	appendRow(new BookmarksItem(BookmarksItem::TrashBookmark, QUrl(), tr("Trash")));
	setItemPrototype(new BookmarksItem(BookmarksItem::UnknownBookmark));
}

QMimeData* BookmarksModel::mimeData(const QModelIndexList &indexes) const
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
		if (indexes.at(i).isValid() && static_cast<BookmarksItem::BookmarkType>(indexes.at(i).data(TypeRole).toInt()) == BookmarksItem::UrlBookmark)
		{
			texts.append(indexes.at(i).data(UrlRole).toString());
			urls.append(indexes.at(i).data(UrlRole).toUrl());
		}
	}

	mimeData->setText(texts.join(QLatin1String(", ")));
	mimeData->setUrls(urls);

	return mimeData;
}

BookmarksItem* BookmarksModel::getRootItem()
{
	return dynamic_cast<BookmarksItem*>(item(0, 0));
}

BookmarksItem* BookmarksModel::getTrashItem()
{
	return dynamic_cast<BookmarksItem*>(item(1, 0));
}

QStringList BookmarksModel::mimeTypes() const
{
	return QStringList(QLatin1String("text/uri-list"));
}

QList<QStandardItem*> BookmarksModel::findUrls(const QString &url, QStandardItem *branch)
{
	if (!branch)
	{
		branch = item(0, 0);
	}

	QList<QStandardItem*> items;

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i);

		if (item)
		{
			const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(item->data(TypeRole).toInt());

			if (type == BookmarksItem::FolderBookmark)
			{
				items.append(findUrls(url, item));
			}
			else if (type == BookmarksItem::UrlBookmark && item->data(UrlRole).toUrl().toString() == url)
			{
				items.append(item);
			}
		}
	}

	return items;
}

bool BookmarksModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(parent.data(BookmarksModel::TypeRole).toInt());

	if (type == BookmarksItem::FolderBookmark || type == BookmarksItem::RootBookmark || type == BookmarksItem::TrashBookmark)
	{
		const QModelIndex index = data->property("x-item-index").toModelIndex();

		if (index.isValid())
		{
			QStandardItem *source = itemFromIndex(index);
			QStandardItem *target = itemFromIndex(parent);
			int targetRow = row;

			if (source && target)
			{
				if (source->parent() == target && source->row() < row)
				{
					--targetRow;
				}

				target->insertRow(targetRow, source->parent()->takeRow(source->row()));

				return true;
			}

			return false;
		}

		return QStandardItemModel::dropMimeData(data, action, row, column, parent);
	}

	return false;
}

}
