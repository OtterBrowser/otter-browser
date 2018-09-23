/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Menu.h"
#include "Action.h"
#include "ContentsWidget.h"
#include "ImportDialog.h"
#include "MainWindow.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/Console.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/SearchEnginesManager.h"
#include "../core/SessionModel.h"
#include "../core/SessionsManager.h"
#include "../core/ThemesManager.h"
#include "../core/ToolBarsManager.h"
#include "../core/Utils.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMetaEnum>
#include <QtCore/QMimeDatabase>
#include <QtCore/QTextCodec>
#include <QtGui/QMouseEvent>

namespace Otter
{

int Menu::m_menuRoleIdentifierEnumerator(-1);

Menu::Menu(int role, QWidget *parent) : QMenu(parent),
	m_actionGroup(nullptr),
	m_clickedAction(nullptr),
	m_role(role),
	m_option(-1)
{
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "File"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Edit"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "View"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "History"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Tools"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Help"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Page"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Print"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Settings"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Frame"))

	switch (role)
	{
		case BookmarksMenu:
		case BookmarkSelectorMenu:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Bookmarks"));
				installEventFilter(this);

				const Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
				{
					connect(BookmarksManager::getModel(), &BookmarksModel::modelModified, this, &Menu::clearBookmarksMenu);
				}

				if (role == BookmarksMenu)
				{
					connect(this, &Menu::aboutToShow, this, &Menu::populateBookmarksMenu);
				}
				else
				{
					connect(this, &Menu::aboutToShow, this, &Menu::populateBookmarkSelectorMenu);
				}
			}

			break;
		case CharacterEncodingMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Character Encoding"));

			m_option = SettingsManager::Content_DefaultCharacterEncodingOption;

			connect(this, &Menu::aboutToShow, this, &Menu::populateCharacterEncodingMenu);
			connect(this, &Menu::triggered, this, &Menu::selectOption);

			break;
		case ClosedWindowsMenu:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Closed Tabs and Windows"));
				setIcon(ThemesManager::createIcon(QLatin1String("user-trash")));

				const MainWindow *mainWindow(MainWindow::findMainWindow(parent));

				if (mainWindow)
				{
					setEnabled(!SessionsManager::getClosedWindows().isEmpty() || !mainWindow->getClosedWindows().isEmpty());

					connect(mainWindow, &MainWindow::closedWindowsAvailableChanged, this, &Menu::updateClosedWindowsMenu);
				}

				connect(SessionsManager::getInstance(), &SessionsManager::closedWindowsChanged, this, &Menu::updateClosedWindowsMenu);
				connect(this, &Menu::aboutToShow, this, &Menu::populateClosedWindowsMenu);
			}

			break;
		case DictionariesMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Dictionaries"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateDictionariesMenu);

			break;
		case ImportExportMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Import and Export"));
			addAction(new Action(-1, {{QLatin1String("importer"), QLatin1String("OperaBookmarks")}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Import Opera Bookmarks…")}}, ActionExecutor::Object(), this));
			addAction(new Action(-1, {{QLatin1String("importer"), QLatin1String("HtmlBookmarks")}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Import HTML Bookmarks…")}}, ActionExecutor::Object(), this));
			addSeparator();
			addAction(new Action(-1, {{QLatin1String("importer"), QLatin1String("OpmlFeeds")}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Import OPML Feeds…")}}, ActionExecutor::Object(), this));
			addSeparator();
			addAction(new Action(-1, {{QLatin1String("importer"), QLatin1String("OperaNotes")}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Import Opera Notes…")}}, ActionExecutor::Object(), this));
			addSeparator();
			addAction(new Action(-1, {{QLatin1String("importer"), QLatin1String("OperaSearchEngines")}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Import Opera Search Engines…")}}, ActionExecutor::Object(), this));
			addSeparator();
			addAction(new Action(-1, {{QLatin1String("importer"), QLatin1String("OperaSession")}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Import Opera Session…")}}, ActionExecutor::Object(), this));

			if (SessionsManager::isReadOnly())
			{
				for (int i = 0; i < actions().count(); ++i)
				{
					QAction *action(actions().at(i));

					if (action && !action->isSeparator())
					{
						action->setEnabled(false);
					}
				}
			}

			connect(this, &Menu::triggered, this, [&](QAction *action)
			{
				if (action)
				{
					ImportDialog::createDialog(qobject_cast<Action*>(action)->getParameters().value(QLatin1String("importer")).toString(), MainWindow::findMainWindow(this));
				}
			});

			break;
		case NotesMenu:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Insert Note"));

				const Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
				{
					connect(NotesManager::getModel(), &BookmarksModel::modelModified, this, &Menu::clearBookmarksMenu);
				}

				connect(this, &Menu::aboutToShow, this, &Menu::populateNotesMenu);
			}

			break;
		case OpenInApplicationMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Open with"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateOpenInApplicationMenu);

			break;
		case ProxyMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Proxy"));

			m_option = SettingsManager::Network_ProxyOption;

			connect(this, &Menu::aboutToShow, this, &Menu::populateProxiesMenu);
			connect(this, &Menu::triggered, this, &Menu::selectOption);

			break;
		case SearchMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Search Using"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateSearchMenu);

			break;
		case SessionsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Sessions"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateSessionsMenu);
			connect(this, &Menu::triggered, this, [&](QAction *action)
			{
				if (!action->data().isNull())
				{
					SessionsManager::restoreSession(SessionsManager::getSession(action->data().toString()), (SettingsManager::getOption(SettingsManager::Sessions_OpenInExistingWindowOption).toBool() ? Application::getActiveWindow() : nullptr));
				}
			});

			break;
		case StyleSheetsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Style"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateStyleSheetsMenu);
			connect(this, &Menu::triggered, this, [&](QAction *action)
			{
				const MainWindow *mainWindow(MainWindow::findMainWindow(parentWidget()));

				if (mainWindow && action)
				{
					Window *window(mainWindow->getActiveWindow());

					if (window && window->getWebWidget())
					{
						window->getWebWidget()->setActiveStyleSheet(action->data().isNull() ? action->text() : QString());
					}
				}
			});

			break;
		case ToolBarsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Toolbars"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateToolBarsMenu);

			break;
		case UserAgentMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "User Agent"));

			m_option = SettingsManager::Network_UserAgentOption;

			connect(this, &Menu::aboutToShow, this, &Menu::populateUserAgentMenu);
			connect(this, &Menu::triggered, this, &Menu::selectOption);

			break;
		case ValidateMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Validate Using"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateSearchMenu);

			break;
		case WindowsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Tabs and Windows"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateWindowsMenu);

			break;
		default:
			break;
	}
}

void Menu::changeEvent(QEvent *event)
{
	QMenu::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && !m_title.isEmpty())
	{
		QMenu::setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));
	}
}

void Menu::hideEvent(QHideEvent *event)
{
	m_clickedAction = nullptr;

	QMenu::hideEvent(event);
}

void Menu::mousePressEvent(QMouseEvent *event)
{
	m_clickedAction = actionAt(event->pos());

	QMenu::mousePressEvent(event);
}

void Menu::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_role == BookmarksMenu && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton))
	{
		const Action *action(qobject_cast<Action*>(actionAt(event->pos())));

		if (action && action == m_clickedAction && action->isEnabled() && action->getIdentifier() == ActionsManager::OpenBookmarkAction)
		{
			QWidget *menu(this);

			while (menu)
			{
				menu->close();
				menu = menu->parentWidget();

				if (!menu || !menu->inherits("QMenu"))
				{
					break;
				}
			}

			const BookmarksModel::Bookmark *bookmark(BookmarksManager::getModel()->getBookmark(action->getParameters().value(QLatin1String("bookmark")).toULongLong()));

			if (bookmark)
			{
				Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::DefaultOpen, event->button(), event->modifiers()))}}, parentWidget());
			}

			m_clickedAction = nullptr;

			return;
		}
	}

	m_clickedAction = nullptr;

	QMenu::mouseReleaseEvent(event);
}

void Menu::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_role == BookmarksMenu)
	{
		const Action *action(qobject_cast<Action*>(actionAt(event->pos())));

		if (action && action->isEnabled() && action->getIdentifier() == ActionsManager::OpenBookmarkAction)
		{
			const quint64 identifier(action->getParameters().value(QLatin1String("bookmark")).toULongLong());
			MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
			ActionExecutor::Object executor(mainWindow, mainWindow);
			QMenu menu(this);
			menu.addAction(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}}, {{QLatin1String("icon"), QLatin1String("document-open")}, {QLatin1String("text"), QCoreApplication::translate("actions", "Open")}}, executor, &menu));
			menu.addAction(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewTabOpen)}}, {{QLatin1String("icon"), {}}, {QLatin1String("text"), QCoreApplication::translate("actions", "Open in New Tab")}}, executor, &menu));
			menu.addAction(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen)}}, {{QLatin1String("icon"), {}}, {QLatin1String("text"), QCoreApplication::translate("actions", "Open in New Background Tab")}}, executor, &menu));
			menu.addSeparator();
			menu.addAction(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewWindowOpen)}}, {{QLatin1String("icon"), {}}, {QLatin1String("text"), QCoreApplication::translate("actions", "Open in New Window")}}, executor, &menu));
			menu.addAction(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen)}}, {{QLatin1String("icon"), {}}, {QLatin1String("text"), QCoreApplication::translate("actions", "Open in New Background Window")}}, executor, &menu));

			connect(&menu, &QMenu::triggered, this, &Menu::hideMenu);

			menu.exec(event->globalPos());

			return;
		}
	}

	QMenu::contextMenuEvent(event);
}

void Menu::load(const QString &path, const QStringList &includeSections, ActionExecutor::Object executor)
{
	QFile file(SessionsManager::getReadableDataPath(path));

	if (file.open(QIODevice::ReadOnly))
	{
		load(QJsonDocument::fromJson(file.readAll()).object(), includeSections, executor);

		file.close();
	}
}

void Menu::load(const QJsonObject &definition, const QStringList &includeSections, ActionExecutor::Object executor)
{
	const QString identifier(definition.value(QLatin1String("identifier")).toString());

	if (m_role == UnknownMenu)
	{
		clear();
	}

	setObjectName(identifier);
	setTitle(definition.value(QLatin1String("title")).toString());

	m_executor = executor;

	executor = getExecutor();

	const QJsonArray actions(definition.value(QLatin1String("actions")).toArray());

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i).isObject() && actions.at(i).toObject().value(QLatin1String("type")) == QLatin1String("include"))
		{
			const QJsonObject object(actions.at(i).toObject());

			if (canInclude(object, includeSections))
			{
				const QJsonArray subActions(object.value(QLatin1String("actions")).toArray());

				for (int j = 0; j < subActions.count(); ++j)
				{
					appendAction(subActions.at(j), includeSections, executor);
				}
			}
		}
		else
		{
			appendAction(actions.at(i), includeSections, executor);
		}
	}
}

void Menu::load(int option)
{
	m_option = option;

	switch (option)
	{
		case SettingsManager::Network_CookiesKeepModeOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Keep Cookie Until"));

			break;
		case SettingsManager::Network_CookiesPolicyOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Accept Cookies"));

			break;
		case SettingsManager::Network_ThirdPartyCookiesPolicyOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Accept Third-party Cookies"));

			break;
		case SettingsManager::Permissions_EnableFullScreenOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Full Screen"));

			break;
		case SettingsManager::Permissions_EnableGeolocationOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Geolocation"));

			break;
		case SettingsManager::Permissions_EnableImagesOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Images"));

			break;
		case SettingsManager::Permissions_EnableMediaCaptureAudioOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Capture Audio"));

			break;
		case SettingsManager::Permissions_EnableMediaCaptureVideoOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Capture Video"));

			break;
		case SettingsManager::Permissions_EnableMediaPlaybackAudioOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Playback Audio"));

			break;
		case SettingsManager::Permissions_EnableNotificationsOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Notifications"));

			break;
		case SettingsManager::Permissions_EnablePluginsOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Plugins"));

			break;
		case SettingsManager::Permissions_EnablePointerLockOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Pointer Lock"));

			break;
		case SettingsManager::Permissions_ScriptsCanCloseWindowsOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Closing Windows by JavaScript"));

			break;
		case SettingsManager::Permissions_ScriptsCanOpenWindowsOption:
			setTitle(QT_TRANSLATE_NOOP("actions", "Pop-Ups"));

			break;
		default:
			setTitle(SettingsManager::getOptionName(option));

			break;
	}

	connect(this, &Menu::aboutToShow, this, &Menu::populateOptionMenu);
	connect(this, &Menu::triggered, this, &Menu::selectOption);
}

void Menu::appendAction(const QJsonValue &definition, const QStringList &sections, ActionExecutor::Object executor)
{
	if (definition.isObject())
	{
		const QJsonObject object(definition.toObject());

		if (!canInclude(object, sections))
		{
			return;
		}

		const QString type(object.value(QLatin1String("type")).toString());
		const QVariantMap options(object.value(QLatin1String("options")).toVariant().toMap());
		const QVariantMap parameters(object.value(QLatin1String("parameters")).toVariant().toMap());

		if (type == QLatin1String("action"))
		{
			const QString rawIdentifier(object.value(QLatin1String("identifier")).toString());
			const int identifier(ActionsManager::getActionIdentifier(rawIdentifier));

			if (identifier < 0)
			{
				Console::addMessage(tr("Failed to create menu action: %1").arg(rawIdentifier), Console::OtherCategory, Console::ErrorLevel);

				return;
			}

			QVariantMap mutableOptions(options);

			if (object.contains(QLatin1String("icon")))
			{
				mutableOptions[QLatin1String("icon")] = object.value(QLatin1String("icon")).toString();
			}

			if (object.contains(QLatin1String("title")))
			{
				mutableOptions[QLatin1String("text")] = object.value(QLatin1String("title")).toString();
			}

			Action *action(new Action(identifier, parameters, mutableOptions, executor, this));

			if (object.contains(QLatin1String("group")))
			{
				const QString group(object.value(QLatin1String("group")).toString());

				if (m_actionGroups.contains(group))
				{
					m_actionGroups[group]->addAction(action);
				}
				else
				{
					QActionGroup *actionGroup(new QActionGroup(this));
					actionGroup->setExclusive(true);
					actionGroup->addAction(action);

					m_actionGroups[group] = actionGroup;
				}
			}

			addAction(action);
		}
		else if (type == QLatin1String("include"))
		{
			const QJsonArray actions(object.value(QLatin1String("actions")).toArray());

			for (int i = 0; i < actions.count(); ++i)
			{
				appendAction(actions.at(i), sections, executor);
			}
		}
		else if (type == QLatin1String("menu"))
		{
			Menu *menu(new Menu(getMenuRoleIdentifier(object.value(QLatin1String("identifier")).toString()), this));
			menu->setExecutor(m_executor);
			menu->setActionParameters(parameters);
			menu->setMenuOptions(options);

			if (object.contains(QLatin1String("actions")))
			{
				menu->load(object, sections, executor);
			}
			else if (options.contains(QLatin1String("option")))
			{
				menu->load(SettingsManager::getOptionIdentifier(options.value(QLatin1String("option")).toString()));
			}

			addMenu(menu);
		}
	}
	else
	{
		const QString rawIdentifier(definition.toString());

		if (rawIdentifier == QLatin1String("separator"))
		{
			addSeparator();
		}
		else
		{
			const int role(rawIdentifier.endsWith(QLatin1String("Menu")) ? getMenuRoleIdentifier(rawIdentifier) : UnknownMenu);

			if (role == UnknownMenu)
			{
				const int identifier(ActionsManager::getActionIdentifier(rawIdentifier));

				if (identifier >= 0)
				{
					addAction(new Action(identifier, {}, executor, this));
				}
				else
				{
					Console::addMessage(tr("Failed to create menu action: %1").arg(rawIdentifier), Console::OtherCategory, Console::ErrorLevel);
				}
			}
			else
			{
				Menu *menu(new Menu(role, this));
				menu->setExecutor(m_executor);

				addMenu(menu);
			}
		}
	}
}

void Menu::hideMenu()
{
	QWidget *menu(this);

	while (menu)
	{
		menu->close();
		menu = menu->parentWidget();

		if (!menu || !menu->inherits("QMenu"))
		{
			break;
		}
	}
}

void Menu::populateBookmarksMenu()
{
	const BookmarksModel::Bookmark *folderBookmark(BookmarksManager::getModel()->getBookmark(m_menuOptions.value(QLatin1String("bookmark")).toULongLong()));

	if (!actions().isEmpty() && !(folderBookmark->getType() == BookmarksModel::RootBookmark && actions().count() == 3))
	{
		return;
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	ActionExecutor::Object executor(mainWindow, mainWindow);

	if (folderBookmark->rowCount() > 1)
	{
		addAction(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), folderBookmark->getIdentifier()}}, {{QLatin1String("icon"), QLatin1String("document-open-folder")}, {QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Open All")}}, executor, this));
		addSeparator();
	}

	for (int i = 0; i < folderBookmark->rowCount(); ++i)
	{
		const BookmarksModel::Bookmark *bookmark(folderBookmark->getChild(i));

		if (!bookmark)
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(bookmark->getType());

		if (type == BookmarksModel::FeedBookmark || type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark || type == BookmarksModel::RootBookmark)
		{
			Action *action(new Action(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}}, {{QLatin1String("text"), Utils::elideText(bookmark->getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), fontMetrics(), this)}}, executor, this));
			action->setToolTip(bookmark->getDescription());
			action->setStatusTip(bookmark->getUrl().toString());

			if (type != BookmarksModel::UrlBookmark)
			{
				if (bookmark->rowCount() > 0)
				{
					Menu *menu(new Menu(BookmarksMenu, this));
					menu->setMenuOptions({{QLatin1String("bookmark"), bookmark->getIdentifier()}});

					action->setMenu(menu);
				}
				else
				{
					action->setEnabled(false);
				}
			}

			addAction(action);
		}
		else
		{
			addSeparator();
		}
	}
}

void Menu::populateBookmarkSelectorMenu()
{
	if (!actions().isEmpty())
	{
		return;
	}

	const BookmarksModel::Bookmark *folderBookmark(BookmarksManager::getModel()->getBookmark(m_menuOptions.value(QLatin1String("bookmark")).toULongLong()));
	Action *addFolderAction(new Action(-1, {}, {{QLatin1String("icon"), QLatin1String("document-open-folder")}, {QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "This Folder")}}, ActionExecutor::Object(), this));
	addFolderAction->setData(folderBookmark->getIdentifier());

	addAction(addFolderAction);
	addSeparator();

	for (int i = 0; i < folderBookmark->rowCount(); ++i)
	{
		const BookmarksModel::Bookmark *bookmark(folderBookmark->getChild(i));

		if (!bookmark)
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(bookmark->getType());

		if (type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark || type == BookmarksModel::RootBookmark)
		{
			QAction *action(new QAction(bookmark->getIcon(), Utils::elideText(bookmark->getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), fontMetrics(), this), this));
			action->setStatusTip(bookmark->getUrl().toString());
			action->setData(bookmark->getIdentifier());

			if (type != BookmarksModel::UrlBookmark)
			{
				Menu *menu(new Menu(BookmarkSelectorMenu, this));
				menu->setMenuOptions({{QLatin1String("bookmark"), bookmark->getIdentifier()}});

				action->setMenu(menu);
			}

			addAction(action);
		}
		else
		{
			addSeparator();
		}
	}
}

void Menu::populateOptionMenu()
{
	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString value(mainWindow ? mainWindow->getOption(m_option).toString() : QString());

	if (m_actionGroup)
	{
		for (int i = 0; i < actions().count(); ++i)
		{
			QAction *action(actions().at(i));

			if (action && action->data().toString() == value)
			{
				action->setChecked(true);

				break;
			}
		}

		return;
	}

	const QVector<SettingsManager::OptionDefinition::Choice> choices(SettingsManager::getOptionDefinition(m_option).choices);

	if (choices.isEmpty())
	{
		return;
	}

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	for (int i = 0; i < choices.count(); ++i)
	{
		QString text;

		if (choices.at(i).value == QLatin1String("ask"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Ask What to Do");
		}
		else if (choices.at(i).value == QLatin1String("allow"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Always Allow");
		}
		else if (choices.at(i).value == QLatin1String("disallow"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Always Deny");
		}
		else if (choices.at(i).value == QLatin1String("keepUntilExpires"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Expires");
		}
		else if (choices.at(i).value == QLatin1String("keepUntilExit"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Current Session is Closed");
		}
		else if (choices.at(i).value == QLatin1String("acceptAll"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Always");
		}
		else if (choices.at(i).value == QLatin1String("acceptExisting"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Only Existing");
		}
		else if (choices.at(i).value == QLatin1String("readOnly"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Only Read Existing");
		}
		else if (choices.at(i).value == QLatin1String("ignore"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Ignore");
		}
		else if (choices.at(i).value == QLatin1String("openAll"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Open All");
		}
		else if (choices.at(i).value == QLatin1String("openAllInBackground"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Open in Background");
		}
		else if (choices.at(i).value == QLatin1String("blockAll"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Block All");
		}
		else if (choices.at(i).value == QLatin1String("onlyCached"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Only Cached");
		}
		else if (choices.at(i).value == QLatin1String("enabled"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Enabled");
		}
		else if (choices.at(i).value == QLatin1String("onDemand"))
		{
			text = QT_TRANSLATE_NOOP("actions", "On Demand");
		}
		else if (choices.at(i).value == QLatin1String("disabled"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Disabled");
		}
		else
		{
			text = choices.at(i).getTitle();
		}

		Action *action(new Action(-1, {}, {{QLatin1String("icon"), choices.at(i).icon}, {QLatin1String("text"), text}}, ActionExecutor::Object(), this));
		action->setCheckable(true);
		action->setChecked(choices.at(i).value == value);
		action->setData(choices.at(i).value);

		m_actionGroup->addAction(action);

		addAction(action);
	}
}

void Menu::populateCharacterEncodingMenu()
{
	if (!m_actionGroup)
	{
		const QVector<int> textCodecs({106, 1015, 1017, 4, 5, 6, 7, 8, 82, 10, 85, 12, 13, 109, 110, 112, 2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 18, 39, 17, 38, 2026});

		m_actionGroup = new QActionGroup(this);
		m_actionGroup->setExclusive(true);

		Action *defaultAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Auto Detect")}}, ActionExecutor::Object(), this));
		defaultAction->setData(QLatin1String("auto"));
		defaultAction->setCheckable(true);

		m_actionGroup->addAction(defaultAction);

		addAction(defaultAction);
		addSeparator();

		for (int i = 0; i < textCodecs.count(); ++i)
		{
			const QTextCodec *codec(QTextCodec::codecForMib(textCodecs.at(i)));

			if (!codec)
			{
				continue;
			}

			QAction *textCodecAction(addAction(Utils::elideText(codec->name(), fontMetrics(), this)));
			textCodecAction->setData(codec->name().toLower());
			textCodecAction->setCheckable(true);

			m_actionGroup->addAction(textCodecAction);
		}
	}

	if (actions().isEmpty())
	{
		return;
	}

	actions().first()->setChecked(true);

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString encoding(mainWindow ? mainWindow->getOption(SettingsManager::Content_DefaultCharacterEncodingOption).toString().toLower() : QString());

	for (int i = 2; i < actions().count(); ++i)
	{
		QAction *action(actions().at(i));

		if (action && action->data().toString() == encoding)
		{
			action->setChecked(true);

			break;
		}
	}
}

void Menu::populateClosedWindowsMenu()
{
	clear();

	Action *clearAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Clear")}}, ActionExecutor::Object(), this));
	clearAction->setEnabled(false);
	clearAction->setIcon(ThemesManager::createIcon(QLatin1String("edit-clear")));

	addAction(clearAction);
	addSeparator();

	const QStringList windows(SessionsManager::getClosedWindows());

	if (!windows.isEmpty())
	{
		ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());

		for (int i = 0; i < windows.count(); ++i)
		{
			QVariantMap parameters;

			if (i > 0)
			{
				parameters[QLatin1String("index")] = i;
			}

			addAction(new Action(ActionsManager::ReopenWindowAction, parameters, {{QLatin1String("text"), Utils::elideText(tr("Window - %1").arg(windows.at(i)), fontMetrics(), this)}}, executor, this));
		}

		addSeparator();

		clearAction->setEnabled(true);
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		const QVector<ClosedWindow> tabs(mainWindow->getClosedWindows());

		if (!tabs.isEmpty())
		{
			ActionExecutor::Object executor(mainWindow, mainWindow);

			for (int i = 0; i < tabs.count(); ++i)
			{
				QVariantMap parameters;

				if (i > 0)
				{
					parameters = {{QLatin1String("index"), i}};
				}

				Action *action(new Action(ActionsManager::ReopenTabAction, parameters, {{QLatin1String("icon"), (tabs.at(i).isPrivate ? QVariant(QLatin1String("tab-private")) : tabs.at(i).icon)}, {QLatin1String("text"), Utils::elideText(tabs.at(i).window.getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), fontMetrics(), this)}}, executor, this));
				action->setStatusTip(tabs.at(i).window.getUrl());

				addAction(action);
			}

			clearAction->setEnabled(true);
		}
	}

	connect(clearAction, &QAction::triggered, this, &Menu::clearClosedWindows);
}

void Menu::populateDictionariesMenu()
{
	clear();

	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();
	}

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	WebWidget *webWidget(nullptr);

	if (m_executor.isValid())
	{
		webWidget = qobject_cast<WebWidget*>(m_executor.getObject());

		if (!webWidget)
		{
			webWidget = m_executor.getObject()->findChild<WebWidget*>();
		}
	}

	const QVector<SpellCheckManager::DictionaryInformation> dictionaries(webWidget ? webWidget->getDictionaries() : SpellCheckManager::getDictionaries());

	for (int i = 0; i < dictionaries.count(); ++i)
	{
		Action *action(new Action(ActionsManager::CheckSpellingAction, {{QLatin1String("dictionary"), dictionaries.at(i).name}}, m_executor, this));

		addAction(action);

		m_actionGroup->addAction(action);
	}
}

void Menu::populateNotesMenu()
{
	if (!actions().isEmpty())
	{
		return;
	}

	const BookmarksModel::Bookmark *folderBookmark(NotesManager::getModel()->getBookmark(m_menuOptions.value(QLatin1String("bookmark")).toULongLong()));

	for (int i = 0; i < folderBookmark->rowCount(); ++i)
	{
		const BookmarksModel::Bookmark *bookmark(folderBookmark->getChild(i));

		if (!bookmark)
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(bookmark->getType());

		if (type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark || type == BookmarksModel::RootBookmark)
		{
			Action *action(new Action(ActionsManager::PasteAction, {{QLatin1String("note"), bookmark->getIdentifier()}}, {{QLatin1String("icon"), bookmark->getIcon()}, {QLatin1String("text"), Utils::elideText(bookmark->getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), fontMetrics(), this)}}, getExecutor(), this));
			action->setToolTip(bookmark->getDescription());
			action->setStatusTip(bookmark->getUrl().toString());

			if (type == BookmarksModel::FolderBookmark)
			{
				if (bookmark->rowCount() > 0)
				{
					Menu *menu(new Menu(NotesMenu, this));
					menu->setMenuOptions({{QLatin1String("bookmark"), bookmark->getIdentifier()}});

					action->setMenu(menu);
				}
				else
				{
					action->setEnabled(false);
				}
			}

			addAction(action);
		}
		else
		{
			addSeparator();
		}
	}
}

void Menu::populateOpenInApplicationMenu()
{
	const QVector<ApplicationInformation> applications(Utils::getApplicationsForMimeType(QMimeDatabase().mimeTypeForName(m_menuOptions.value(QLatin1String("mimeType")).toString())));
	const ActionExecutor::Object executor(getExecutor());
	QVariantMap parameters(m_actionParameters);

	if (applications.isEmpty())
	{
		parameters[QLatin1String("application")] = QString();

		addAction(new Action(ActionsManager::OpenUrlAction, parameters, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Default Application")}}, executor, this));
	}
	else
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			parameters[QLatin1String("application")] = applications.at(i).command;

			addAction(new Action(ActionsManager::OpenUrlAction, parameters, {{QLatin1String("icon"), applications.at(i).icon}, {QLatin1String("text"), ((applications.at(i).name.isEmpty()) ? QT_TRANSLATE_NOOP("actions", "Unknown") : applications.at(i).name)}}, executor, this));

			if (i == 0)
			{
				addSeparator();
			}
		}
	}

	disconnect(this, &Menu::aboutToShow, this, &Menu::populateOpenInApplicationMenu);
}

void Menu::populateProxiesMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();

		clear();
	}

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString proxy(mainWindow ? mainWindow->getOption(SettingsManager::Network_ProxyOption).toString() : QString());
	const QStringList proxies((!menuAction() || menuAction()->data().toString().isEmpty()) ? NetworkManagerFactory::getProxies() : NetworkManagerFactory::getProxy(menuAction()->data().toString()).children);

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	for (int i = 0; i < proxies.count(); ++i)
	{
		if (proxies.at(i).isEmpty())
		{
			addSeparator();
		}
		else
		{
			const ProxyDefinition definition(NetworkManagerFactory::getProxy(proxies.at(i)));
			Action *action(new Action(-1, {}, {{QLatin1String("text"), Utils::elideText(definition.getTitle(), fontMetrics(), this)}}, ActionExecutor::Object(), this));
			action->setData(proxies.at(i));

			if (definition.isFolder)
			{
				action->setIcon(ThemesManager::createIcon(QLatin1String("inode-directory")));

				if (definition.children.count() > 0)
				{
					action->setMenu(new Menu(m_role, this));
				}
				else
				{
					action->setEnabled(false);
				}
			}
			else
			{
				action->setCheckable(true);
				action->setChecked(proxy == proxies.at(i));
			}

			m_actionGroup->addAction(action);

			addAction(action);
		}
	}
}

void Menu::populateSearchMenu()
{
	clear();

	const QStringList searchEngines((m_role == ValidateMenu) ? SettingsManager::getOption(SettingsManager::Browser_ValidatorsOrderOption).toStringList() : SearchEnginesManager::getSearchEngines());
	ActionExecutor::Object executor(getExecutor());

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

		if (searchEngine.isValid())
		{
			Action *action(new Action(ActionsManager::SearchAction, {{QLatin1String("searchEngine"), searchEngine.identifier}, {QLatin1String("queryPlaceholder"), ((m_role == ValidateMenu) ? QLatin1String("{pageUrl}") : QLatin1String("{selection}"))}}, {{QLatin1String("icon"), searchEngine.icon}, {QLatin1String("text"), searchEngine.title}}, executor, this));
			action->setToolTip(searchEngine.description);

			addAction(action);
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

	ActionExecutor::Object executor(getExecutor());

	addAction(new Action(ActionsManager::SaveSessionAction, {}, executor, this));
	addAction(new Action(ActionsManager::SessionsAction, {}, executor, this));
	addSeparator();

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	const QStringList sessions(SessionsManager::getSessions());
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session(SessionsManager::getSession(sessions.at(i)));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted(information.values());
	const QString currentSession(SessionsManager::getCurrentSession());

	for (int i = 0; i < sorted.count(); ++i)
	{
		int windows(0);

		for (int j = 0; j < sorted.at(i).windows.count(); ++j)
		{
			windows += sorted.at(i).windows.at(j).windows.count();
		}

		QAction *action(addAction(tr("%1 (%n tab(s))", "", windows).arg(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : QString(sorted.at(i).title).replace(QLatin1Char('&'), QLatin1String("&&")))));
		action->setData(sorted.at(i).path);
		action->setCheckable(true);
		action->setChecked(sorted.at(i).path == currentSession);

		m_actionGroup->addAction(action);
	}
}

void Menu::populateStyleSheetsMenu()
{
	clear();

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	Action *defaultAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Default Style")}}, ActionExecutor::Object(), this));
	defaultAction->setData(-1);
	defaultAction->setCheckable(true);
	defaultAction->setChecked(true);

	addAction(defaultAction);

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getActiveWindow());

	if (!window || !window->getWebWidget())
	{
		return;
	}

	addSeparator();

	const QString activeStyleSheet(window->getWebWidget()->getActiveStyleSheet());
	const QStringList styleSheets(window->getWebWidget()->getStyleSheets());
	QActionGroup *actionGroup(new QActionGroup(this));
	actionGroup->setExclusive(true);
	actionGroup->addAction(defaultAction);

	for (int i = 0; i < styleSheets.count(); ++i)
	{
		QAction *action(addAction(styleSheets.at(i)));
		action->setCheckable(true);
		action->setChecked(styleSheets.at(i) == activeStyleSheet);

		actionGroup->addAction(action);
	}

	defaultAction->setChecked(activeStyleSheet.isEmpty() || !styleSheets.contains(activeStyleSheet));
}

void Menu::populateToolBarsMenu()
{
	clear();

	ActionExecutor::Object executor(getExecutor());
	const QVector<ToolBarsManager::ToolBarDefinition> definitions(ToolBarsManager::getToolBarDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		addAction(new Action(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::getToolBarName(definitions.at(i).identifier)}}, executor, this));
	}

	addSeparator();

	Menu *addNewMenu(new Menu(UnknownMenu, this));
	Action *addNewAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Add New")}}, ActionExecutor::Object(), this));
	addNewAction->setMenu(addNewMenu);

	Action *addToolBarAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Add Toolbar…")}}, ActionExecutor::Object(), addNewMenu));
	Action *addBookmarksBarAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Add Bookmarks Bar…")}}, ActionExecutor::Object(), addNewMenu));
	Action *addSideBarAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Add Sidebar…")}}, ActionExecutor::Object(), addNewMenu));

	addNewMenu->addAction(addToolBarAction);
	addNewMenu->addAction(addBookmarksBarAction);
	addNewMenu->addAction(addSideBarAction);

	addAction(addNewAction);
	addSeparator();
	addAction(new Action(ActionsManager::LockToolBarsAction, {}, executor, this));
	addSeparator();
	addAction(new Action(ActionsManager::ResetToolBarsAction, {}, executor, this));

	connect(addToolBarAction, &QAction::triggered, addToolBarAction, [&]()
	{
		ToolBarsManager::addToolBar(ToolBarsManager::ActionsBarType);
	});
	connect(addBookmarksBarAction, &QAction::triggered, addBookmarksBarAction, [&]()
	{
		ToolBarsManager::addToolBar(ToolBarsManager::BookmarksBarType);
	});
	connect(addSideBarAction, &QAction::triggered, addSideBarAction, [&]()
	{
		ToolBarsManager::addToolBar(ToolBarsManager::SideBarType);
	});
}

void Menu::populateUserAgentMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();

		clear();
	}

	const bool isRoot(!menuAction() || menuAction()->data().toString().isEmpty());
	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString userAgent(mainWindow ? mainWindow->getOption(SettingsManager::Network_UserAgentOption).toString() : QString());
	const QStringList userAgents(isRoot ? NetworkManagerFactory::getUserAgents() : NetworkManagerFactory::getUserAgent(menuAction()->data().toString()).children);

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	for (int i = 0; i < userAgents.count(); ++i)
	{
		if (userAgents.at(i).isEmpty())
		{
			addSeparator();
		}
		else
		{
			const UserAgentDefinition definition(NetworkManagerFactory::getUserAgent(userAgents.at(i)));
			QAction *action(addAction(Utils::elideText(definition.getTitle(), fontMetrics(), this)));
			action->setData(userAgents.at(i));

			if (definition.isFolder)
			{
				action->setIcon(ThemesManager::createIcon(QLatin1String("inode-directory")));

				if (definition.children.count() > 0)
				{
					action->setMenu(new Menu(m_role, this));
				}
				else
				{
					action->setEnabled(false);
				}
			}
			else
			{
				action->setCheckable(true);
				action->setChecked(userAgent == userAgents.at(i));
			}

			m_actionGroup->addAction(action);
		}
	}

	if (isRoot)
	{
		addSeparator();

		Action *customAction(new Action(-1, {}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Custom User Agent…")}}, ActionExecutor::Object(), this));
		customAction->setData(QLatin1String("custom"));
		customAction->setCheckable(true);
		customAction->setChecked(userAgent.startsWith(QLatin1String("custom;")));

		m_actionGroup->addAction(customAction);

		addAction(customAction);
	}
}

void Menu::populateWindowsMenu()
{
	if (actions().isEmpty())
	{
		const MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			connect(mainWindow, &MainWindow::titleChanged, this, &Menu::populateWindowsMenu);
			connect(mainWindow, &MainWindow::windowAdded, this, &Menu::populateWindowsMenu);
			connect(mainWindow, &MainWindow::windowRemoved, this, &Menu::populateWindowsMenu);
		}

		disconnect(this, &Menu::aboutToShow, this, &Menu::populateWindowsMenu);
	}

	clear();

	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const MainWindowSessionItem *mainWindowItem(SessionsManager::getModel()->getMainWindowItem(mainWindow));

	if (mainWindowItem)
	{
		ActionExecutor::Object executor(mainWindow, mainWindow);

		for (int i = 0; i < mainWindowItem->rowCount(); ++i)
		{
			const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(mainWindowItem->child(i, 0)));

			if (windowItem)
			{
				const Window *window(windowItem->getActiveWindow());

				addAction(new Action(ActionsManager::ActivateTabAction, {{QLatin1String("tab"), window->getIdentifier()}}, {{QLatin1String("icon"), window->getIcon()}, {QLatin1String("text"), Utils::elideText((window->getTitle().isEmpty() ? QT_TRANSLATE_NOOP("actions", "(Untitled)") : window->getTitle()), fontMetrics(), this)}}, executor, this));
			}
		}
	}
}

void Menu::clearBookmarksMenu()
{
	const int offset((m_menuOptions.value(QLatin1String("bookmark")).toULongLong() == 0) ? 3 : 0);

	for (int i = (actions().count() - 1); i >= offset; --i)
	{
		actions().at(i)->deleteLater();

		removeAction(actions().at(i));
	}

	connect(this, &Menu::aboutToShow, this, &Menu::populateBookmarksMenu);
}

void Menu::clearClosedWindows()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		mainWindow->clearClosedWindows();
	}

	SessionsManager::clearClosedWindows();
}

void Menu::clearNotesMenu()
{
	clear();

	if (menuAction())
	{
		const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(m_menuOptions.value(QLatin1String("bookmark")).toULongLong()));

		menuAction()->setEnabled(bookmark && bookmark->rowCount() > 0);
	}

	connect(this, &Menu::aboutToShow, this, &Menu::populateNotesMenu);
}

void Menu::selectOption(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		mainWindow->setOption(m_option, action->data().toString());
	}
}

void Menu::updateClosedWindowsMenu()
{
	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	setEnabled((mainWindow && mainWindow->getClosedWindows().count() > 0) || SessionsManager::getClosedWindows().count() > 0);
}

void Menu::setTitle(const QString &title)
{
	m_title = title;

	QMenu::setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));
}

void Menu::setExecutor(ActionExecutor::Object executor)
{
	m_executor = executor;
}

void Menu::setActionParameters(const QVariantMap &parameters)
{
	m_actionParameters = parameters;
}

void Menu::setMenuOptions(const QVariantMap &options)
{
	m_menuOptions = options;
}

ActionExecutor::Object Menu::getExecutor() const
{
	if (m_executor.isValid())
	{
		return m_executor;
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parentWidget()));

	if (mainWindow)
	{
		return ActionExecutor::Object(mainWindow, mainWindow);
	}

	return ActionExecutor::Object(Application::getInstance(), Application::getInstance());
}

int Menu::getRole() const
{
	return m_role;
}

int Menu::getMenuRoleIdentifier(const QString &name)
{
	if (m_menuRoleIdentifierEnumerator < 0)
	{
		m_menuRoleIdentifierEnumerator = Menu::staticMetaObject.indexOfEnumerator(QLatin1String("MenuRole").data());
	}

	if (!name.endsWith(QLatin1String("Menu")))
	{
		return Menu::staticMetaObject.enumerator(m_menuRoleIdentifierEnumerator).keyToValue((name + QLatin1String("Menu")).toLatin1());
	}

	return Menu::staticMetaObject.enumerator(m_menuRoleIdentifierEnumerator).keyToValue(name.toLatin1());
}

bool Menu::canInclude(const QJsonObject &definition, const QStringList &sections)
{
	if (definition.contains(QLatin1String("excludeFrom")) && hasIncludeMatch(definition, QLatin1String("excludeFrom"), sections))
	{
		return false;
	}

	if (definition.contains(QLatin1String("includeIn")) && !hasIncludeMatch(definition, QLatin1String("includeIn"), sections))
	{
		return false;
	}

	return true;
}

bool Menu::hasIncludeMatch(const QJsonObject &definition, const QString &key, const QStringList &sections)
{
	if (!definition.contains(key))
	{
		return false;
	}

	const QJsonValue value(definition.value(key));
	const QStringList values((value.type() == QJsonValue::Array) ? value.toVariant().toStringList() : QStringList(value.toString()));

	for (int i = 0; i < values.count(); ++i)
	{
		if (sections.contains(values.at(i)))
		{
			return true;
		}
	}

	return false;
}

}
