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

#include "BookmarkWidget.h"
#include "../../../core/Application.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Menu.h"

#include <QtGui/QMouseEvent>

namespace Otter
{

BookmarkWidget::BookmarkWidget(BookmarksModel::Bookmark *bookmark, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_bookmark(bookmark)
{
	updateBookmark(m_bookmark);

	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkRemoved, this, &BookmarkWidget::removeBookmark);
	connect(BookmarksManager::getModel(), &BookmarksModel::bookmarkModified, this, &BookmarkWidget::updateBookmark);
}

void BookmarkWidget::changeEvent(QEvent *event)
{
	ToolButtonWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && m_bookmark && m_bookmark->getRawData(BookmarksModel::TitleRole).isNull())
	{
		updateBookmark(m_bookmark);
	}
}

void BookmarkWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QToolButton::mouseReleaseEvent(event);

	if ((event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) && m_bookmark)
	{
		Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), m_bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::DefaultOpen, event->button(), event->modifiers()))}}, parentWidget());
	}
}

void BookmarkWidget::removeBookmark(BookmarksModel::Bookmark *bookmark)
{
	if (m_bookmark && m_bookmark == bookmark)
	{
		m_bookmark = nullptr;

		deleteLater();
	}
}

void BookmarkWidget::updateBookmark(BookmarksModel::Bookmark *bookmark)
{
	if (bookmark != m_bookmark)
	{
		return;
	}

	const BookmarksModel::BookmarkType type(m_bookmark->getType());

	if (type == BookmarksModel::RootBookmark || type == BookmarksModel::TrashBookmark || type == BookmarksModel::FeedBookmark || type == BookmarksModel::FolderBookmark)
	{
		if (!menu())
		{
			Menu *menu(new Menu(Menu::BookmarksMenu, this));
			menu->setMenuOptions({{QLatin1String("bookmark"), m_bookmark->getIdentifier()}});

			setMenu(menu);
		}

		setPopupMode(QToolButton::InstantPopup);
		setToolTip(m_bookmark->getTitle());
		setEnabled(m_bookmark->rowCount() > 0);
	}
	else
	{
		QStringList toolTip;
		toolTip.append(tr("Title: %1").arg(m_bookmark->getTitle()));

		if (!m_bookmark->getUrl().isEmpty())
		{
			toolTip.append(tr("Address: %1").arg(m_bookmark->getUrl().toDisplayString()));
		}

		if (!m_bookmark->getDescription().isEmpty())
		{
			toolTip.append(tr("Description: %1").arg(m_bookmark->getDescription()));
		}

		if (m_bookmark->getTimeAdded().isValid())
		{
			toolTip.append(tr("Created: %1").arg(Utils::formatDateTime(m_bookmark->getTimeAdded())));
		}

		if (m_bookmark->getTimeVisited().isValid() && type != BookmarksModel::FeedBookmark)
		{
			toolTip.append(tr("Visited: %1").arg(Utils::formatDateTime(m_bookmark->getTimeVisited())));
		}

		setToolTip(QLatin1String("<div style=\"white-space:pre;\">") + toolTip.join(QLatin1Char('\n')) + QLatin1String("</div>"));
		setMenu(nullptr);
	}

	setIcon(getIcon());
	setStatusTip(m_bookmark->getUrl().toDisplayString());
	setText(getText().replace(QLatin1Char('&'), QLatin1String("&&")));
}

QString BookmarkWidget::getText() const
{
	const QVariantMap options(getOptions());

	return ((isCustomized() && options.contains(QLatin1String("text"))) ? options[QLatin1String("text")].toString() : m_bookmark->getTitle());
}

QIcon BookmarkWidget::getIcon() const
{
	const QVariantMap options(getOptions());

	if (isCustomized() && options.contains(QLatin1String("icon")))
	{
		const QVariant iconData(options[QLatin1String("icon")]);

		if (iconData.type() == QVariant::Icon)
		{
			return iconData.value<QIcon>();
		}

		return ThemesManager::createIcon(iconData.toString());
	}

	return m_bookmark->getIcon();
}

}
