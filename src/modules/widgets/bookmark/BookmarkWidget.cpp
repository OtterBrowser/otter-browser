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

#include "BookmarkWidget.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/Utils.h"
#include "../../../core/WindowsManager.h"
#include "../../../ui/Menu.h"

#include <QtCore/QDateTime>
#include <QtGui/QMouseEvent>

namespace Otter
{

BookmarkWidget::BookmarkWidget(BookmarksItem *bookmark, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_bookmark(bookmark)
{
	updateBookmark(m_bookmark);

	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*,BookmarksItem*)), this, SLOT(removeBookmark(BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkModified(BookmarksItem*)), this, SLOT(updateBookmark(BookmarksItem*)));
}

BookmarkWidget::BookmarkWidget(const QString &path, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_bookmark(BookmarksManager::getModel()->getItem(path))
{
	updateBookmark(m_bookmark);

	connect(BookmarksManager::getModel(), SIGNAL(bookmarkRemoved(BookmarksItem*,BookmarksItem*)), this, SLOT(removeBookmark(BookmarksItem*)));
	connect(BookmarksManager::getModel(), SIGNAL(bookmarkModified(BookmarksItem*)), this, SLOT(updateBookmark(BookmarksItem*)));
}

void BookmarkWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QToolButton::mouseReleaseEvent(event);

	if ((event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) && m_bookmark)
	{
		ActionsManager::triggerAction(ActionsManager::OpenBookmarkAction, parent(), {{QLatin1String("bookmark"), m_bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant(WindowsManager::calculateOpenHints(WindowsManager::DefaultOpen, event->button(), event->modifiers()))}});
	}
}

void BookmarkWidget::removeBookmark(BookmarksItem *bookmark)
{
	if (m_bookmark && m_bookmark == bookmark)
	{
		m_bookmark = nullptr;

		deleteLater();
	}
}

void BookmarkWidget::updateBookmark(BookmarksItem *bookmark)
{
	if (bookmark != m_bookmark)
	{
		return;
	}

	const QString title(m_bookmark->data(BookmarksModel::TitleRole).toString().isEmpty() ? tr("(Untitled)") : m_bookmark->data(BookmarksModel::TitleRole).toString());
	const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(m_bookmark->data(BookmarksModel::TypeRole).toInt()));

	if (type == BookmarksModel::RootBookmark || type == BookmarksModel::TrashBookmark || type == BookmarksModel::FolderBookmark)
	{
		Menu *menu(new Menu(Menu::BookmarksMenuRole, this));
		menu->menuAction()->setData(m_bookmark->index());

		setPopupMode(QToolButton::InstantPopup);
		setToolTip(title);
		setMenu(menu);
		setEnabled(m_bookmark->rowCount() > 0);
	}
	else
	{
		QStringList toolTip;
		toolTip.append(tr("Title: %1").arg(title));

		if (!m_bookmark->data(BookmarksModel::UrlRole).toString().isEmpty())
		{
			toolTip.append(tr("Address: %1").arg(m_bookmark->data(BookmarksModel::UrlRole).toString()));
		}

		if (m_bookmark->data(BookmarksModel::DescriptionRole).isValid())
		{
			toolTip.append(tr("Description: %1").arg(m_bookmark->data(BookmarksModel::DescriptionRole).toString()));
		}

		if (!m_bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().isNull())
		{
			toolTip.append(tr("Created: %1").arg(m_bookmark->data(BookmarksModel::TimeAddedRole).toDateTime().toString()));
		}

		if (!m_bookmark->data(BookmarksModel::TimeVisitedRole).toDateTime().isNull())
		{
			toolTip.append(tr("Visited: %1").arg(m_bookmark->data(BookmarksModel::TimeVisitedRole).toDateTime().toString()));
		}

		setToolTip(QLatin1String("<div style=\"white-space:pre;\">") + toolTip.join(QLatin1Char('\n')) + QLatin1String("</div>"));
		setMenu(nullptr);
	}

	setText(title);
	setStatusTip(m_bookmark->data(BookmarksModel::UrlRole).toString());
	setIcon(m_bookmark->data(Qt::DecorationRole).value<QIcon>());
}

}
