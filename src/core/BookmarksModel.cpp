/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "AddonsManager.h"
#include "Utils.h"
#include "WebBackend.h"

#include <QtCore/QMimeData>

namespace Otter
{

QHash<QUrl, QList<BookmarksItem*> > BookmarksItem::m_urls;
QHash<QString, BookmarksItem*> BookmarksItem::m_keywords;
QMap<quint64, BookmarksItem*> BookmarksItem::m_identifiers;

BookmarksItem::BookmarksItem(BookmarkType type, quint64 identifier, const QUrl &url, const QString &title) : QStandardItem()
{
	setData(type, BookmarksModel::TypeRole);
	setData(url, BookmarksModel::UrlRole);
	setData(title, BookmarksModel::TitleRole);

	if (type != SeparatorBookmark && type != TrashBookmark && type != UnknownBookmark)
	{
		if (identifier == 0 || m_identifiers.contains(identifier))
		{
			identifier = (m_identifiers.isEmpty() ? 1 : (m_identifiers.keys().last() + 1));
		}

		setData(identifier, BookmarksModel::IdentifierRole);

		m_identifiers[identifier] = this;
	}

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
	const quint64 identifier = data(BookmarksModel::IdentifierRole).toULongLong();

	if (identifier > 0 && m_identifiers.contains(identifier))
	{
		m_identifiers.remove(identifier);
	}

	if (!data(BookmarksModel::UrlRole).toString().isEmpty())
	{
		const QUrl url = adjustUrl(data(BookmarksModel::UrlRole).toUrl());

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
		const QUrl oldUrl = adjustUrl(data(BookmarksModel::UrlRole).toUrl());
		const QUrl newUrl = adjustUrl(value.toUrl());

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
	BookmarksItem *item = new BookmarksItem(static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt()), 0, data(BookmarksModel::UrlRole).toUrl(), data(BookmarksModel::TitleRole).toString());
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

BookmarksItem* BookmarksItem::getBookmark(quint64 identifier)
{
	if (m_identifiers.contains(identifier))
	{
		return m_identifiers[identifier];
	}

	return NULL;
}

QUrl BookmarksItem::adjustUrl(QUrl url)
{
	url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments | QUrl::StripTrailingSlash);

	if (url.path() == QLatin1String("/"))
	{
		url.setPath(QString());
	}

	return url;
}

QStringList BookmarksItem::getKeywords()
{
	return m_keywords.keys();
}

QList<BookmarksItem*> BookmarksItem::getBookmarks(const QUrl &url)
{
	const QUrl adjustedUrl = adjustUrl(url);

	if (m_urls.contains(adjustedUrl))
	{
		return m_urls[adjustedUrl];
	}

	return QList<BookmarksItem*>();
}

QList<QUrl> BookmarksItem::getUrls()
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

		if (type == TrashBookmark)
		{
			return Utils::getIcon(QLatin1String("user-trash"));
		}

		if (type == UrlBookmark)
		{
			return AddonsManager::getWebBackend()->getIconForUrl(data(BookmarksModel::UrlRole).toUrl());
		}

		return QVariant();
	}

	if (role == Qt::AccessibleDescriptionRole && static_cast<BookmarkType>(data(BookmarksModel::TypeRole).toInt()) == SeparatorBookmark)
	{
		return QLatin1String("separator");
	}

	return QStandardItem::data(role);
}

bool BookmarksItem::hasBookmark(const QUrl &url)
{
	return m_urls.contains(adjustUrl(url));
}

bool BookmarksItem::hasKeyword(const QString &keyword)
{
	return m_keywords.contains(keyword);
}

bool BookmarksItem::hasUrl(const QUrl &url)
{
	return m_urls.contains(adjustUrl(url));
}

BookmarksModel::BookmarksModel(QObject *parent) : QStandardItemModel(parent)
{
	appendRow(new BookmarksItem(BookmarksItem::RootBookmark, 0, QUrl(), tr("Bookmarks")));
	appendRow(new BookmarksItem(BookmarksItem::TrashBookmark, 0, QUrl(), tr("Trash")));
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

BookmarksItem* BookmarksModel::getItem(const QString &path)
{
	if (path == QLatin1String("/"))
	{
		return getRootItem();
	}

	QStandardItem *item = getRootItem();
	const QStringList directories = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

	for (int i = 0; i < directories.count(); ++i)
	{
		bool found = false;

		for (int j = 0; j < item->rowCount(); ++j)
		{
			if (item->child(j) && item->child(j)->data(Qt::DisplayRole) == directories.at(i))
			{
				item = item->child(j);

				found = true;

				break;
			}
		}

		if (!found)
		{
			return NULL;
		}
	}

	return dynamic_cast<BookmarksItem*>(item);
}

QStringList BookmarksModel::mimeTypes() const
{
	return QStringList(QLatin1String("text/uri-list"));
}

QList<QStandardItem*> BookmarksModel::findUrls(const QUrl &url, QStandardItem *branch)
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
			else if (type == BookmarksItem::UrlBookmark && item->data(UrlRole).toUrl() == url)
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

			if (source && target)
			{
				if (row < 0)
				{
					target->appendRow(source->parent()->takeRow(source->row()));

					return true;
				}

				int targetRow = row;

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
