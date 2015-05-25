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

#include "Menu.h"
#include "ImportDialog.h"
#include "MainWindow.h"
#include "../core/ActionsManager.h"
#include "../core/BookmarksManager.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/SessionsManager.h"
#include "../core/ToolBarsManager.h"
#include "../core/Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QTextCodec>
#include <QtGui/QMouseEvent>

namespace Otter
{

Menu::Menu(MenuRole role, QWidget *parent) : QMenu(parent),
	m_actionGroup(NULL),
	m_bookmark(NULL),
	m_role(role)
{
	switch (role)
	{
		case BookmarksMenuRole:
		case BookmarkSelectorMenuRole:
		case NotesMenuRole:
			{
				installEventFilter(this);

				Menu *parentMenu = qobject_cast<Menu*>(parent);

				if (!parentMenu || (parentMenu && parentMenu->getRole() != m_role))
				{
					if (m_role == NotesMenuRole)
					{
						connect(NotesManager::getModel(), SIGNAL(modelModified()), this, SLOT(clearModelMenu()));
					}
					else
					{
						connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(clearModelMenu()));
					}
				}

				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateModelMenu()));
			}

			break;

		case CharacterEncodingMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateCharacterEncodingMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectCharacterEncoding(QAction*)));

			break;
		case ClosedWindowsMenu:
			{
				setIcon(Utils::getIcon(QLatin1String("user-trash")));

				MainWindow *window = MainWindow::findMainWindow(parent);

				if (window)
				{
					setEnabled(!SessionsManager::getClosedWindows().isEmpty() || !window->getWindowsManager()->getClosedWindows().isEmpty());

					connect(window->getWindowsManager(), SIGNAL(closedWindowsAvailableChanged(bool)), this, SLOT(updateClosedWindowsMenu()));
				}

				connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindowsMenu()));
				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateClosedWindowsMenu()));
			}

			break;
		case ImportExportMenuRole:
			QMenu::addAction(tr("Import Opera Bookmarks"))->setData(QLatin1String("OperaBookmarks"));
			QMenu::addAction(tr("Import HTML Bookmarks"))->setData(QLatin1String("HtmlBookmarks"));

			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openImporter(QAction*)));

			break;
		case SessionsMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateSessionsMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openSession(QAction*)));

			break;

		case ToolBarsMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateToolBarsMenu()));

			break;
		case UserAgentMenuRole:
			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateUserAgentMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectUserAgent(QAction*)));

			break;
		default:
			break;
	}
}

void Menu::changeEvent(QEvent *event)
{
	QMenu::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));

			break;
		default:
			break;
	}
}

void Menu::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_role == BookmarksMenuRole && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton))
	{
		QAction *action = actionAt(event->pos());

		if (action && action->data().type() == QVariant::ModelIndex)
		{
			QWidget *menu = this;

			while (menu)
			{
				menu->close();
				menu = menu->parentWidget();

				if (!menu || !menu->inherits(QLatin1String("QMenu").data()))
				{
					break;
				}
			}

			MainWindow *window = MainWindow::findMainWindow(parent());

			if (window)
			{
				window->getWindowsManager()->open(dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(action->data().toModelIndex())), WindowsManager::calculateOpenHints(event->modifiers(), event->button()));

				return;
			}
		}
	}

	QMenu::mouseReleaseEvent(event);
}

void Menu::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_role == BookmarksMenuRole)
	{
		QAction *action = actionAt(event->pos());

		if (action && action->data().type() == QVariant::ModelIndex)
		{
			m_bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(action->data().toModelIndex()));

			QMenu contextMenu(this);
			contextMenu.addAction(Utils::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
			contextMenu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(NewTabOpen);
			contextMenu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(NewBackgroundTabOpen);
			contextMenu.addSeparator();
			contextMenu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(NewWindowOpen);
			contextMenu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(NewBackgroundWindowOpen);
			contextMenu.exec(event->globalPos());

			return;
		}
	}

	QMenu::contextMenuEvent(event);
}

void Menu::load(const QJsonObject &definition)
{
	const QString identifier = definition.value(QLatin1String("identifier")).toString();

	if (m_role == NoMenuRole)
	{
		clear();
	}

	m_title = definition.value(QLatin1String("title")).toString();

	setObjectName(identifier);
	setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));

	const QJsonArray actions = definition.value(QLatin1String("actions")).toArray();

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i).isObject())
		{
			const QJsonObject object = actions.at(i).toObject();
			Menu *menu = new Menu(Menu::getRole(object.value(QLatin1String("identifier")).toString()), this);
			menu->load(object);

			addMenu(menu);
		}
		else
		{
			const QString rawAction = actions.at(i).toString();

			if (rawAction == QLatin1String("separator"))
			{
				addSeparator();
			}
			else
			{
				const int action = ActionsManager::getActionIdentifier(rawAction);

				if (action >= 0)
				{
					QMenu::addAction(ActionsManager::getAction(action, parentWidget()));
				}
			}
		}
	}
}

void Menu::populateModelMenu()
{
	QMenu *menu = qobject_cast<QMenu*>(sender());

	if (!menu || !menu->menuAction())
	{
		return;
	}

	QModelIndex index = menu->menuAction()->data().toModelIndex();

	if (!menu->actions().isEmpty() && !(m_role == BookmarksMenuRole && !index.isValid() && menu->actions().count() == 3))
	{
		return;
	}

	if (!index.isValid())
	{
		index = ((m_role == NotesMenuRole) ? NotesManager::getModel() : BookmarksManager::getModel())->getRootItem()->index();
	}

	const QAbstractItemModel *model = index.model();

	if (!model)
	{
		return;
	}

	if (model->rowCount(index) > 1 && m_role == BookmarksMenuRole)
	{
		QAction *openAllAction = menu->addAction(Utils::getIcon(QLatin1String("document-open-folder")), tr("Open All"), this, SLOT(openBookmark()));
		openAllAction->setData(index);

		menu->addSeparator();
	}

	for (int i = 0; i < model->rowCount(index); ++i)
	{
		const QModelIndex childIndex = index.child(i, 0);

		if (!childIndex.isValid())
		{
			continue;
		}

		const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(childIndex.data(BookmarksModel::TypeRole).toInt());

		if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark)
		{
			QAction *action = menu->addAction(childIndex.data(Qt::DecorationRole).value<QIcon>(), (childIndex.data(BookmarksModel::TitleRole).toString().isEmpty() ? tr("(Untitled)") : Utils::elideText(QString(childIndex.data(BookmarksModel::TitleRole).toString()).replace(QLatin1Char('&'), QLatin1String("&&")), menu)));
			action->setData(childIndex);
			action->setToolTip(childIndex.data(BookmarksModel::DescriptionRole).toString());
			action->setStatusTip(childIndex.data(BookmarksModel::UrlRole).toString());

			if (type == BookmarksModel::UrlBookmark && m_role == BookmarksMenuRole)
			{
				connect(action, SIGNAL(triggered()), this, SLOT(openBookmark()));
			}
			else if (type == BookmarksModel::FolderBookmark)
			{
				if (model->rowCount(childIndex) > 0)
				{
					action->setMenu(new Menu(m_role, this));
				}
				else
				{
					action->setEnabled(false);
				}
			}
		}
		else
		{
			menu->addSeparator();
		}
	}
}

void Menu::populateCharacterEncodingMenu()
{
	if (!m_actionGroup)
	{
		QList<int> textCodecs;
		textCodecs << 106 << 1015 << 1017 << 4 << 5 << 6 << 7 << 8 << 82 << 10 << 85 << 12 << 13 << 109 << 110 << 112 << 2250 << 2251 << 2252 << 2253 << 2254 << 2255 << 2256 << 2257 << 2258 << 18 << 39 << 17 << 38 << 2026;

		m_actionGroup = new QActionGroup(this);
		m_actionGroup->setExclusive(true);

		QAction *defaultAction = QMenu::addAction(tr("Auto Detect"));
		defaultAction->setData(-1);
		defaultAction->setCheckable(true);

		m_actionGroup->addAction(defaultAction);

		addSeparator();

		for (int i = 0; i < textCodecs.count(); ++i)
		{
			QTextCodec *codec = QTextCodec::codecForMib(textCodecs.at(i));

			if (!codec)
			{
				continue;
			}

			QAction *textCodecAction = QMenu::addAction(Utils::elideText(codec->name(), this));
			textCodecAction->setData(textCodecs.at(i));
			textCodecAction->setCheckable(true);

			m_actionGroup->addAction(textCodecAction);
		}
	}

	MainWindow *window = MainWindow::findMainWindow(parent());
	const QString encoding = (window ? window->getWindowsManager()->getOption(QLatin1String("Content/DefaultCharacterEncoding")).toString().toLower() : QString());

	for (int i = 2; i < actions().count(); ++i)
	{
		QAction *action = actions().at(i);

		if (!action)
		{
			continue;
		}

		action->setChecked(encoding == action->text().toLower());

		if (action->isChecked())
		{
			break;
		}
	}

	if (!m_actionGroup->checkedAction() && !actions().isEmpty())
	{
		actions().first()->setChecked(true);
	}
}

void Menu::populateClosedWindowsMenu()
{
	clear();

	QMenu::addAction(Utils::getIcon(QLatin1String("edit-clear")), tr("Clear"), this, SLOT(clearClosedWindows()))->setData(0);

	addSeparator();

	const QStringList windows = SessionsManager::getClosedWindows();

	if (!windows.isEmpty())
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			QMenu::addAction(Utils::elideText(tr("Window - %1").arg(windows.at(i)), this), this, SLOT(restoreClosedWindow()))->setData(-(i + 1));
		}

		addSeparator();
	}

	MainWindow *window = MainWindow::findMainWindow(parent());

	if (window)
	{
		const QList<ClosedWindow> tabs = window->getWindowsManager()->getClosedWindows();

		for (int i = 0; i < tabs.count(); ++i)
		{
			QAction *action = QMenu::addAction(HistoryManager::getIcon(QUrl(tabs.at(i).window.getUrl())), Utils::elideText(tabs.at(i).window.getTitle(), this), this, SLOT(restoreClosedWindow()));
			action->setData(i + 1);
			action->setStatusTip(tabs.at(i).window.getUrl());
		}
	}
}

void Menu::populateSessionsMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();
	}

	clear();

	QMenu::addAction(ActionsManager::getAction(ActionsManager::SaveSessionAction, parent()));
	QMenu::addAction(ActionsManager::getAction(ActionsManager::SessionsAction, parent()));

	addSeparator();

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	const QStringList sessions = SessionsManager::getSessions();
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session = SessionsManager::getSession(sessions.at(i));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted = information.values();
	const QString currentSession = SessionsManager::getCurrentSession();

	for (int i = 0; i < sorted.count(); ++i)
	{
		int windows = 0;

		for (int j = 0; j < sorted.at(i).windows.count(); ++j)
		{
			windows += sorted.at(i).windows.at(j).windows.count();
		}

		QAction *action = QMenu::addAction(tr("%1 (%n tab(s))", "", windows).arg(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : QString(sorted.at(i).title).replace(QLatin1Char('&'), QLatin1String("&&"))));
		action->setData(sorted.at(i).path);
		action->setCheckable(true);
		action->setChecked(sorted.at(i).path == currentSession);

		m_actionGroup->addAction(action);
	}
}

void Menu::populateToolBarsMenu()
{
	clear();

	const QVector<ToolBarDefinition> definitions = ToolBarsManager::getToolBarDefinitions();

	for (int i = 0; i < definitions.count(); ++i)
	{
		QAction *toolBarAction = QMenu::addAction(definitions.at(i).title.isEmpty() ? tr("(Untitled)") : definitions.at(i).title);
		toolBarAction->setData(definitions.at(i).identifier);
		toolBarAction->setCheckable(true);
		toolBarAction->setChecked(definitions.at(i).visibility != AlwaysHiddenToolBar);

		connect(toolBarAction, SIGNAL(toggled(bool)), this, SLOT(setToolBarVisibility(bool)));
	}

	addAction(ActionsManager::ShowSidebarAction, true);
	addAction(ActionsManager::ShowErrorConsoleAction, true);
	addSeparator();

	QMenu *menu = addMenu(tr("Add New"));
	menu->addAction(tr("Add Toolbar"), ToolBarsManager::getInstance(), SLOT(addToolBar()));
	menu->addAction(tr("Add Bookmarks Bar…"), ToolBarsManager::getInstance(), SLOT(addBookmarksBar()));

	addSeparator();
	addAction(ActionsManager::LockToolBarsAction, true);
	addSeparator();
	QMenu::addAction(tr("Reset to Defaults…"), ToolBarsManager::getInstance(), SLOT(resetToolBars()));
}

void Menu::populateUserAgentMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();

		clear();
	}

	MainWindow *window = MainWindow::findMainWindow(parent());
	const QStringList userAgents = NetworkManagerFactory::getUserAgents();
	const QString userAgent = (window ? window->getWindowsManager()->getOption(QLatin1String("Network/UserAgent")).toString() : QString());

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	QAction *defaultAction = QMenu::addAction(tr("Default"));
	defaultAction->setData(QLatin1String("default"));
	defaultAction->setCheckable(true);
	defaultAction->setChecked(userAgent == QLatin1String("default"));

	m_actionGroup->addAction(defaultAction);

	addSeparator();

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const QString title = NetworkManagerFactory::getUserAgent(userAgents.at(i)).title;
		QAction *userAgentAction = QMenu::addAction((title.isEmpty() ? tr("(Untitled)") : Utils::elideText(title, this)));
		userAgentAction->setData(userAgents.at(i));
		userAgentAction->setCheckable(true);
		userAgentAction->setChecked(userAgent == userAgents.at(i));

		m_actionGroup->addAction(userAgentAction);
	}

	addSeparator();

	QAction *customAction = QMenu::addAction(tr("Custom"));
	customAction->setData(QLatin1String("custom"));
	customAction->setCheckable(true);
	customAction->setChecked(userAgent.startsWith(QLatin1String("custom;")));

	m_actionGroup->addAction(customAction);

	if (!m_actionGroup->checkedAction() && actions().count() > 2)
	{
		actions().first()->setChecked(true);
	}
}

void Menu::clearModelMenu()
{
	const int offset = ((m_role == BookmarksMenuRole && menuAction() && !menuAction()->data().toModelIndex().isValid()) ? 3 : 0);

	for (int i = (actions().count() - 1); i >= offset; --i)
	{
		actions().at(i)->deleteLater();

		removeAction(actions().at(i));
	}

	if (m_role != BookmarksMenuRole && menuAction())
	{
		const QAbstractItemModel *model = menuAction()->data().toModelIndex().model();

		if (model)
		{
			menuAction()->setEnabled(model->rowCount(menuAction()->data().toModelIndex()) > 0);
		}
	}

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateModelMenu()));
}

void Menu::clearClosedWindows()
{
	MainWindow *window = MainWindow::findMainWindow(parent());

	if (window)
	{
		window->getWindowsManager()->clearClosedWindows();
	}

	SessionsManager::clearClosedWindows();
}

void Menu::restoreClosedWindow()
{
	QAction *action = qobject_cast<QAction*>(sender());
	const int index = ((action && action->data().type() == QVariant::Int) ? action->data().toInt() : 0);

	if (index > 0)
	{
		MainWindow *window = MainWindow::findMainWindow(parent());

		if (window)
		{
			window->getWindowsManager()->restore(index - 1);
		}
	}
	else if (index < 0)
	{
		SessionsManager::restoreClosedWindow(-index - 1);
	}
}

void Menu::openBookmark()
{
	QWidget *menu = this;

	while (menu)
	{
		menu->close();
		menu = menu->parentWidget();

		if (!menu || !menu->inherits(QLatin1String("QMenu").data()))
		{
			break;
		}
	}

	MainWindow *window = MainWindow::findMainWindow(parent());
	QAction *action = qobject_cast<QAction*>(sender());

	if (action && action->data().type() == QVariant::ModelIndex)
	{
		m_bookmark = dynamic_cast<BookmarksItem*>(BookmarksManager::getModel()->itemFromIndex(action->data().toModelIndex()));
	}

	if (window)
	{
		const OpenHints hints = (action ? static_cast<OpenHints>(action->data().toInt()) : DefaultOpen);

		window->getWindowsManager()->open(m_bookmark, ((hints == DefaultOpen) ? WindowsManager::calculateOpenHints() : hints));
	}

	m_bookmark = NULL;
}

void Menu::openImporter(QAction *action)
{
	if (action)
	{
		ImportDialog::createDialog(action->data().toString(), this);
	}
}

void Menu::openSession(QAction *action)
{
	if (!action->data().isNull())
	{
		SessionsManager::restoreSession(SessionsManager::getSession(action->data().toString()));
	}
}

void Menu::selectCharacterEncoding(QAction *action)
{
	MainWindow *window = MainWindow::findMainWindow(parent());

	if (!window)
	{
		return;
	}

	QString encoding;

	if (action && action->data().toInt() > 0)
	{
		QTextCodec *codec = QTextCodec::codecForMib(action->data().toInt());

		if (codec)
		{
			encoding = codec->name();
		}
	}

	window->getWindowsManager()->setOption(QLatin1String("Content/DefaultCharacterEncoding"), encoding.toLower());
}

void Menu::selectUserAgent(QAction *action)
{
	MainWindow *window = MainWindow::findMainWindow(parent());

	if (action && window)
	{
		window->getWindowsManager()->setOption(QLatin1String("Network/UserAgent"), action->data().toString());
	}
}

void Menu::updateClosedWindowsMenu()
{
	MainWindow *window = MainWindow::findMainWindow(parent());

	setEnabled((window && window->getWindowsManager()->getClosedWindows().count() > 0) || SessionsManager::getClosedWindows().count() > 0);
}

void Menu::setToolBarVisibility(bool visible)
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		ToolBarDefinition definition = ToolBarsManager::getToolBarDefinition(action->data().toInt());
		definition.visibility = (visible ? AlwaysVisibleToolBar : AlwaysHiddenToolBar);

		ToolBarsManager::setToolBar(definition);
	}
}

Action* Menu::addAction(int identifier, bool isGlobal)
{
	Action *action = (isGlobal ? ActionsManager::getAction(identifier, this) : new Action(identifier, this));

	QMenu::addAction(action);

	return action;
}

Menu::MenuRole Menu::getRole() const
{
	return m_role;
}

Menu::MenuRole Menu::getRole(const QString &identifier)
{
	if (identifier == QLatin1String("BookmarksMenu"))
	{
		return BookmarksMenuRole;
	}

	if (identifier == QLatin1String("CharacterEncodingMenu"))
	{
		return CharacterEncodingMenuRole;
	}

	if (identifier == QLatin1String("ClosedWindowsMenu"))
	{
		return ClosedWindowsMenu;
	}

	if (identifier == QLatin1String("ImportExportMenu"))
	{
		return ImportExportMenuRole;
	}

	if (identifier == QLatin1String("SessionsMenu"))
	{
		return SessionsMenuRole;
	}

	if (identifier == QLatin1String("ToolBarsMenu"))
	{
		return ToolBarsMenuRole;
	}

	if (identifier == QLatin1String("UserAgentMenu"))
	{
		return UserAgentMenuRole;
	}

	return NoMenuRole;
}

}
