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

#include "WebWidget.h"
#include "ContentsWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtWidgets/QMenu>

namespace Otter
{

WebWidget::WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : QWidget(parent),
	m_backend(backend),
	m_quickSearchMenu(NULL)
{
	Q_UNUSED(isPrivate)

	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(updateQuickSearch()));
}

void WebWidget::search(const QString &query, const QString &engine)
{
	Q_UNUSED(query)
	Q_UNUSED(engine)
}

void WebWidget::quickSearch(QAction *action)
{
	const QString engine = ((!action || action->data().type() != QVariant::String) ? getQuickSearchEngine() : action->data().toString());

	if (SearchesManager::getSearchEngines().contains(engine))
	{
		if (engine != m_quickSearchEngine)
		{
			m_quickSearchEngine = engine;

			if (m_quickSearchMenu)
			{
				m_quickSearchMenu->clear();
			}

			emit quickSearchEngineChanged();
		}

		emit requestedSearch(getSelectedText(), m_quickSearchEngine);
	}
}

void WebWidget::quickSearchMenuAboutToShow()
{
	if (m_quickSearchMenu && m_quickSearchMenu->isEmpty())
	{
		const QStringList engines = SearchesManager::getSearchEngines();

		for (int i = 0; i < engines.count(); ++i)
		{
			SearchInformation *engine = SearchesManager::getSearchEngine(engines.at(i));

			if (engine)
			{
				QAction *action = m_quickSearchMenu->addAction(engine->icon, engine->title);
				action->setData(engine->identifier);
				action->setToolTip(engine->description);
			}
		}
	}
}

void WebWidget::showContextMenu(const QPoint &position, MenuFlags flags)
{
	QMenu menu;

	if (flags & StandardMenu)
	{
		menu.addAction(getAction(GoBackAction));
		menu.addAction(getAction(GoForwardAction));
		menu.addAction(getAction(RewindAction));
		menu.addAction(getAction(FastForwardAction));
		menu.addSeparator();
		menu.addAction(getAction(ReloadAction));
		menu.addAction(getAction(ReloadTimeAction));
		menu.addSeparator();
		menu.addAction(getAction(BookmarkAction));
		menu.addAction(getAction(CopyAddressAction));
		menu.addAction(getAction(PrintAction));
		menu.addSeparator();

		if (flags & FormMenu)
		{
			menu.addAction(getAction(CreateSearchAction));
			menu.addSeparator();
		}

		menu.addAction(getAction(InspectElementAction));
		menu.addAction(getAction(ViewSourceAction));
		menu.addAction(getAction(ValidateAction));
		menu.addSeparator();

		if (flags & FrameMenu)
		{
			QMenu *frameMenu = new QMenu(&menu);
			frameMenu->setTitle(tr("Frame"));
			frameMenu->addAction(getAction(OpenFrameInThisTabAction));
			frameMenu->addAction(getAction(OpenFrameInNewTabAction));
			frameMenu->addAction(getAction(OpenFrameInNewTabBackgroundAction));
			frameMenu->addSeparator();
			frameMenu->addAction(getAction(ViewSourceFrameAction));
			frameMenu->addAction(getAction(ReloadFrameAction));
			frameMenu->addAction(getAction(CopyFrameLinkToClipboardAction));

			menu.addMenu(frameMenu);
			menu.addSeparator();
		}

		menu.addAction(getAction(ContentBlockingAction));
		menu.addAction(getAction(WebsitePreferencesAction));
		menu.addSeparator();
		menu.addAction(ActionsManager::getAction(QLatin1String("FullScreen")));
	}
	else
	{
		if (flags & EditMenu)
		{
			menu.addAction(getAction(UndoAction));
			menu.addAction(getAction(RedoAction));
			menu.addSeparator();
			menu.addAction(getAction(CutAction));
			menu.addAction(getAction(CopyAction));
			menu.addAction(getAction(PasteAction));
			menu.addAction(getAction(DeleteAction));
			menu.addSeparator();
			menu.addAction(getAction(SelectAllAction));
			menu.addAction(getAction(ClearAllAction));
			menu.addSeparator();

			if (flags & FormMenu)
			{
				menu.addAction(getAction(CreateSearchAction));
				menu.addSeparator();
			}

			if (flags == EditMenu || flags == (EditMenu | FormMenu))
			{
				menu.addAction(getAction(InspectElementAction));
				menu.addSeparator();
			}

			menu.addAction(getAction(SpellCheckAction));
			menu.addSeparator();
		}

		if (flags & SelectionMenu)
		{
			menu.addAction(getAction(SearchAction));
			menu.addAction(getAction(SearchMenuAction));
			menu.addSeparator();

			if (!(flags & EditMenu))
			{
				menu.addAction(getAction(CopyAction));
				menu.addSeparator();
			}

			menu.addAction(getAction(OpenSelectionAsLinkAction));
			menu.addSeparator();
		}

		if (flags & MailMenu)
		{
			menu.addAction(getAction(OpenLinkInThisTabAction));
			menu.addSeparator();
			menu.addAction(getAction(CopyLinkToClipboardAction));

			if (!(flags & ImageMenu))
			{
				menu.addAction(getAction(InspectElementAction));
			}

			menu.addSeparator();
		}
		else if (flags & LinkMenu)
		{
			menu.addAction(getAction(OpenLinkInThisTabAction));
			menu.addAction(getAction(OpenLinkInNewTabAction));
			menu.addAction(getAction(OpenLinkInNewTabBackgroundAction));
			menu.addSeparator();
			menu.addAction(getAction(OpenLinkInNewWindowAction));
			menu.addAction(getAction(OpenLinkInNewWindowBackgroundAction));
			menu.addSeparator();
			menu.addAction(getAction(BookmarkLinkAction));
			menu.addAction(getAction(CopyLinkToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(SaveLinkToDiskAction));
			menu.addAction(getAction(SaveLinkToDownloadsAction));

			if (!(flags & ImageMenu))
			{
				menu.addAction(getAction(InspectElementAction));
			}

			menu.addSeparator();
		}

		if (flags & ImageMenu)
		{
			menu.addAction(getAction(OpenImageInNewTabAction));
			menu.addAction(getAction(ReloadImageAction));
			menu.addAction(getAction(CopyImageUrlToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(SaveImageToDiskAction));
			menu.addAction(getAction(CopyImageToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(InspectElementAction));
			menu.addAction(getAction(ImagePropertiesAction));
			menu.addSeparator();
		}

		if (flags & MediaMenu)
		{
			menu.addAction(getAction(CopyMediaUrlToClipboardAction));
			menu.addAction(getAction(SaveMediaToDiskAction));
			menu.addSeparator();
			menu.addAction(getAction(ToggleMediaPlayPauseAction));
			menu.addAction(getAction(ToggleMediaMuteAction));
			menu.addAction(getAction(ToggleMediaLoopAction));
			menu.addAction(getAction(ToggleMediaControlsAction));
			menu.addSeparator();
			menu.addAction(getAction(InspectElementAction));
			menu.addSeparator();
		}
	}

	menu.exec(mapToGlobal(position));
}

void WebWidget::setRequestedUrl(const QUrl &url, bool typed, bool onlyUpdate)
{
	m_requestedUrl = url;

	if (!onlyUpdate)
	{
		setUrl(url, typed);
	}
}

void WebWidget::updateQuickSearch()
{
	if (m_quickSearchMenu && sender() == SearchesManager::getInstance())
	{
		m_quickSearchMenu->clear();
	}

	if (!SearchesManager::getSearchEngines().contains(m_quickSearchEngine))
	{
		const QString engine = SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString();

		if (engine != m_quickSearchEngine)
		{
			m_quickSearchEngine = engine;

			emit quickSearchEngineChanged();
		}
	}
}

void WebWidget::setStatusMessage(const QString &message, bool override)
{
	const QString oldMessage = getStatusMessage();

	if (override)
	{
		m_overridingStatusMessage = message;
	}
	else
	{
		m_javaScriptStatusMessage = message;
	}

	const QString newMessage = getStatusMessage();

	if (newMessage != oldMessage)
	{
		emit statusMessageChanged(newMessage);
	}
}

void WebWidget::setQuickSearchEngine(const QString &engine)
{
	if (engine != m_quickSearchEngine)
	{
		m_quickSearchEngine = engine;

		updateQuickSearch();

		emit quickSearchEngineChanged();
	}
}

WebBackend* WebWidget::getBackend()
{
	return m_backend;
}

QMenu* WebWidget::getQuickSearchMenu()
{
	if (!m_quickSearchMenu)
	{
		m_quickSearchMenu = new QMenu(this);

		connect(m_quickSearchMenu, SIGNAL(aboutToShow()), this, SLOT(quickSearchMenuAboutToShow()));
		connect(m_quickSearchMenu, SIGNAL(triggered(QAction*)), this, SLOT(quickSearch(QAction*)));
	}

	return m_quickSearchMenu;
}

QString WebWidget::getQuickSearchEngine() const
{
	return (m_quickSearchEngine.isEmpty() ? SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString() : m_quickSearchEngine);
}

QString WebWidget::getSelectedText() const
{
	return QString();
}

QString WebWidget::getStatusMessage() const
{
	return (m_overridingStatusMessage.isEmpty() ? m_javaScriptStatusMessage : m_overridingStatusMessage);
}

QUrl WebWidget::getRequestedUrl() const
{
	return ((getUrl().isEmpty() || isLoading()) ?  m_requestedUrl : getUrl());
}

}
