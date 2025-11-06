/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/Console.h"
#include "../core/FeedsManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/SearchEnginesManager.h"
#include "../core/SessionModel.h"
#include "../core/SessionsManager.h"
#include "../core/ThemesManager.h"
#include "../core/ToolBarsManager.h"
#include "../core/Utils.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMetaEnum>
#include <QtCore/QMimeDatabase>
#include <QtGui/QMouseEvent>
#include <QtGui/QScreen>

namespace Otter
{

int Menu::m_menuRoleIdentifierEnumerator(-1);

Menu::Menu(QWidget *parent) : QMenu(parent),
	m_actionGroup(nullptr),
	m_clickedAction(nullptr),
	m_role(UnknownMenu),
	m_option(-1)
{
}

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
		case BookmarkMenu:
			connect(this, &Menu::aboutToShow, this, [&]()
			{
				const quint64 identifier(m_menuOptions[QLatin1String("bookmark")].toULongLong());
				Action *openBookmarkAction(new MenuAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}}, m_executor, this));
				openBookmarkAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open"));
				openBookmarkAction->setIconOverride(QLatin1String("document-open"));

				addAction(openBookmarkAction);

				Action *openBookmarkInNewTabAction(new MenuAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewTabOpen)}}, m_executor, this));
				openBookmarkInNewTabAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open in New Tab"));
				openBookmarkInNewTabAction->setIconOverride(QIcon());

				addAction(openBookmarkInNewTabAction);

				Action *openBookmarkInNewBackgroundTabAction(new MenuAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen)}}, m_executor, this));
				openBookmarkInNewBackgroundTabAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"));
				openBookmarkInNewBackgroundTabAction->setIconOverride(QIcon());

				addAction(openBookmarkInNewBackgroundTabAction);
				addSeparator();

				Action *openBookmarkInNewWindowAction(new MenuAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewWindowOpen)}}, m_executor, this));
				openBookmarkInNewWindowAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open in New Window"));
				openBookmarkInNewWindowAction->setIconOverride(QIcon());

				addAction(openBookmarkInNewWindowAction);

				Action *openBookmarkInNewBackgroundWindowAction(new MenuAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), identifier}, {QLatin1String("hints"), QVariant(SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen)}}, m_executor, this));
				openBookmarkInNewBackgroundWindowAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open in New Background Window"));
				openBookmarkInNewBackgroundWindowAction->setIconOverride(QIcon());

				addAction(openBookmarkInNewBackgroundWindowAction);
			});

			break;
		case BookmarksMenu:
		case BookmarkSelectorMenu:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Bookmarks"));

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
		case FeedsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Feeds"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateFeedsMenu);

			break;
		case ImportExportMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Import and Export"));

			connect(this, &Menu::aboutToShow, this, [&]()
			{
				if (!isEmpty())
				{
					return;
				}

				const ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());
				const QStringList exchangers({QLatin1String("OperaBookmarksImport"), QLatin1String("HtmlBookmarksImport"), {}, QLatin1String("OpmlFeedsImport"), {}, QLatin1String("OperaNotesImport"), {}, QLatin1String("OperaSearchEnginesImport"), {}, QLatin1String("OperaSessionImport"), {}, QLatin1String("HtmlBookmarksExport"), QLatin1String("XbelBookmarksExport")});

				for (int i = 0; i < exchangers.count(); ++i)
				{
					const QString exchanger(exchangers.at(i));

					if (exchanger.isEmpty())
					{
						addSeparator();
					}
					else
					{
						addAction(new MenuAction(ActionsManager::ExchangeDataAction, {{QLatin1String("exchanger"), exchanger}}, executor, this));
					}
				}
			});

			break;
		case NotesMenu:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Insert Note"));

				const Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
				{
					connect(NotesManager::getModel(), &BookmarksModel::modelModified, this, &Menu::clearNotesMenu);
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

			connect(this, &Menu::aboutToShow, this, &Menu::populateSearchEnginesMenu);

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
		case SpellCheckSuggestionsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Spelling Suggestions"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateSpellCheckSuggestionsMenu);

			break;
		case StyleSheetsMenu:
			setTitle(QT_TRANSLATE_NOOP("actions", "Style"));

			connect(this, &Menu::aboutToShow, this, &Menu::populateStyleSheetsMenu);
			connect(this, &Menu::triggered, this, [&](QAction *action)
			{
				const MenuContext context(getMenuContext());

				if (context.webWidget && action)
				{
					context.webWidget->setActiveStyleSheet(action->data().isNull() ? action->text() : QString());
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

			connect(this, &Menu::aboutToShow, this, &Menu::populateSearchEnginesMenu);

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
	const Action *action(qobject_cast<Action*>(actionAt(event->pos())));

	if (m_role != BookmarksMenu || !(event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) || !action || action != m_clickedAction || !action->isEnabled() || action->getIdentifier() != ActionsManager::OpenBookmarkAction)
	{
		m_clickedAction = nullptr;

		QMenu::mouseReleaseEvent(event);

		return;
	}

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
}

void Menu::contextMenuEvent(QContextMenuEvent *event)
{
	const MenuAction *action(qobject_cast<MenuAction*>(actionAt(event->pos())));

	if (!action || !action->isEnabled() || !action->hasContextMenu())
	{
		QMenu::contextMenuEvent(event);

		return;
	}

	QMenu *menu(action->createContextMenu(this));

	connect(menu, &QMenu::triggered, this, &Menu::hideMenu);

	menu->exec(event->globalPos());
	menu->deleteLater();
}

void Menu::load(const QString &path, const QStringList &includeSections, const ActionExecutor::Object &executor)
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
		const QJsonValue action(actions.at(i));
		const QJsonObject object(action.toObject());

		if (!action.isObject() || object.value(QLatin1String("type")) != QLatin1String("include"))
		{
			appendAction(action, includeSections, executor);
		}
		else if (canInclude(object, includeSections))
		{
			const QJsonArray subActions(object.value(QLatin1String("actions")).toArray());

			for (int j = 0; j < subActions.count(); ++j)
			{
				appendAction(subActions.at(j), includeSections, executor);
			}
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

void Menu::appendAction(const QJsonValue &definition, const QStringList &sections, const ActionExecutor::Object &executor)
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

			Action *action(new MenuAction(identifier, parameters, executor, this));

			if (object.contains(QLatin1String("icon")))
			{
				const QVariant data(options[QLatin1String("icon")]);

				if (data.type() == QVariant::Icon)
				{
					action->setIconOverride(data.value<QIcon>());
				}
				else
				{
					action->setIconOverride(data.toString());
				}
			}

			if (object.contains(QLatin1String("title")))
			{
				action->setTextOverride(object.value(QLatin1String("title")).toString(), false);
			}

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
					addAction(new MenuAction(identifier, {}, executor, this));
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

	if (!isEmpty() && !(folderBookmark->getType() == BookmarksModel::RootBookmark && actions().count() == 3))
	{
		return;
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	ActionExecutor::Object executor(mainWindow, mainWindow);

	if (folderBookmark->rowCount() > 1)
	{
		Action *action(new OpenBookmarkMenuAction(folderBookmark->getIdentifier(), executor, this));
		action->setTextOverride(QT_TRANSLATE_NOOP("actions", "Open All"));
		action->setIconOverride(QLatin1String("document-open-folder"));

		addAction(action);
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

		switch (type)
		{
			case BookmarksModel::FeedBookmark:
			case BookmarksModel::FolderBookmark:
			case BookmarksModel::UrlBookmark:
			case BookmarksModel::RootBookmark:
				{
					Action *action(new OpenBookmarkMenuAction(bookmark->getIdentifier(), executor, this));
					action->setTextOverride(bookmark->getTitle(), false);

					if (type != BookmarksModel::UrlBookmark)
					{
						if (bookmark->hasChildren())
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

				break;
			default:
				addSeparator();

				break;
		}
	}
}

void Menu::populateBookmarkSelectorMenu()
{
	if (!isEmpty())
	{
		return;
	}

	const BookmarksModel::Bookmark *folderBookmark(BookmarksManager::getModel()->getBookmark(m_menuOptions.value(QLatin1String("bookmark")).toULongLong()));
	Action *addFolderAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "This Folder"), true, this));
	addFolderAction->setIconOverride(QLatin1String("document-open-folder"));
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

		switch (type)
		{
			case BookmarksModel::FolderBookmark:
			case BookmarksModel::UrlBookmark:
			case BookmarksModel::RootBookmark:
				{
					QAction *action(new QAction(bookmark->getIcon(), bookmark->getTitle(), this));
					action->setStatusTip(bookmark->getUrl().toString());
					action->setData(bookmark->getIdentifier());

					if (bookmark->isFolder())
					{
						Menu *menu(new Menu(BookmarkSelectorMenu, this));
						menu->setMenuOptions({{QLatin1String("bookmark"), bookmark->getIdentifier()}});

						action->setMenu(menu);
					}

					addAction(action);
				}

				break;
			default:
				addSeparator();

				break;
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
		const SettingsManager::OptionDefinition::Choice choice(choices.at(i));
		QString text;

		if (choice.value == QLatin1String("ask"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Ask What to Do");
		}
		else if (choice.value == QLatin1String("allow"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Always Allow");
		}
		else if (choice.value == QLatin1String("disallow"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Always Deny");
		}
		else if (choice.value == QLatin1String("keepUntilExpires"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Expires");
		}
		else if (choice.value == QLatin1String("keepUntilExit"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Current Session is Closed");
		}
		else if (choice.value == QLatin1String("acceptAll"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Always");
		}
		else if (choice.value == QLatin1String("acceptExisting"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Only Existing");
		}
		else if (choice.value == QLatin1String("readOnly"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Only Read Existing");
		}
		else if (choice.value == QLatin1String("ignore"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Ignore");
		}
		else if (choice.value == QLatin1String("openAll"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Open All");
		}
		else if (choice.value == QLatin1String("openAllInBackground"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Open in Background");
		}
		else if (choice.value == QLatin1String("blockAll"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Block All");
		}
		else if (choice.value == QLatin1String("onlyCached"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Only Cached");
		}
		else if (choice.value == QLatin1String("enabled"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Enabled");
		}
		else if (choice.value == QLatin1String("onDemand"))
		{
			text = QT_TRANSLATE_NOOP("actions", "On Demand");
		}
		else if (choice.value == QLatin1String("disabled"))
		{
			text = QT_TRANSLATE_NOOP("actions", "Disabled");
		}
		else
		{
			text = choice.getTitle();
		}

		Action *action(new MenuAction(text, true, this));
		action->setIconOverride(choice.icon);
		action->setCheckable(true);
		action->setChecked(choice.value == value);
		action->setData(choice.value);

		m_actionGroup->addAction(action);

		addAction(action);
	}
}

void Menu::populateCharacterEncodingMenu()
{
	if (!m_actionGroup)
	{
		m_actionGroup = new QActionGroup(this);
		m_actionGroup->setExclusive(true);

		Action *defaultAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Auto Detect"), true, this));
		defaultAction->setData(QLatin1String("auto"));
		defaultAction->setCheckable(true);

		m_actionGroup->addAction(defaultAction);

		addAction(defaultAction);
		addSeparator();

		const QStringList encodings(Utils::getCharacterEncodings());

		for (int i = 0; i < encodings.count(); ++i)
		{
			const QString encoding(encodings.at(i));
			QAction *textCodecAction(addAction(Utils::elideText(encoding, fontMetrics(), this)));
			textCodecAction->setData(encoding.toLower());
			textCodecAction->setCheckable(true);

			m_actionGroup->addAction(textCodecAction);
		}
	}

	if (isEmpty())
	{
		return;
	}

	actions().value(0)->setChecked(true);

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

	Action *clearAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Clear"), true, this));
	clearAction->setIcon(ThemesManager::createIcon(QLatin1String("edit-clear")));
	clearAction->setEnabled(false);

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

			Action *reopenWindowAction(new MenuAction(ActionsManager::ReopenWindowAction, parameters, executor, this));
			reopenWindowAction->setTextOverride(tr("Window - %1").arg(windows.at(i)), false);

			addAction(reopenWindowAction);
		}

		addSeparator();

		clearAction->setEnabled(true);
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		const QVector<Session::ClosedWindow> tabs(mainWindow->getClosedWindows());

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

				const Session::ClosedWindow tab(tabs.at(i));
				Action *reopenTabAction(new MenuAction(ActionsManager::ReopenTabAction, parameters, executor, this));
				reopenTabAction->setTextOverride(tab.window.getTitle(), false);

				if (tab.isPrivate)
				{
					reopenTabAction->setIconOverride(QLatin1String("tab-private"));
				}
				else
				{
					reopenTabAction->setIconOverride(tab.icon);
				}

				addAction(reopenTabAction);
			}

			clearAction->setEnabled(true);
		}
	}

	connect(clearAction, &QAction::triggered, this, [&]()
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

		if (mainWindow)
		{
			mainWindow->clearClosedWindows();
		}

		SessionsManager::clearClosedWindows();
	});
}

void Menu::populateDictionariesMenu()
{
	clear();

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
		addAction(new MenuAction(ActionsManager::CheckSpellingAction, {{QLatin1String("dictionary"), dictionaries.at(i).language}}, m_executor, this));
	}
}

void Menu::populateFeedsMenu()
{
	if (!isEmpty())
	{
		return;
	}

	const FeedsModel::Entry *folderEntry(FeedsManager::getModel()->getEntry(m_menuOptions.value(QLatin1String("entry")).toULongLong()));
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	ActionExecutor::Object executor(mainWindow, mainWindow);

	for (int i = 0; i < folderEntry->rowCount(); ++i)
	{
		const FeedsModel::Entry *entry(folderEntry->getChild(i));

		if (!entry)
		{
			continue;
		}

		const bool isFolder(entry->isFolder());

		if (isFolder || entry->getType() == FeedsModel::FeedEntry)
		{
			QString text(entry->getTitle().replace(QLatin1Char('&'), QLatin1String("&&")));
			const int unreadEntriesAmount(entry->data(FeedsModel::UnreadEntriesAmountRole).toInt());

			if (unreadEntriesAmount > 0)
			{
				text.append(QStringLiteral(" (%1)").arg(unreadEntriesAmount));
			}

			Action *action(new MenuAction(ActionsManager::OpenFeedAction, {{QLatin1String("entry"), entry->getIdentifier()}}, executor, this));
			action->setTextOverride(text, false);

			if (isFolder)
			{
				if (entry->rowCount() > 0)
				{
					Menu *menu(new Menu(FeedsMenu, this));
					menu->setMenuOptions({{QLatin1String("entry"), entry->getIdentifier()}});

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

void Menu::populateNotesMenu()
{
	if (!isEmpty())
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

		switch (type)
		{
			case BookmarksModel::FolderBookmark:
			case BookmarksModel::UrlBookmark:
			case BookmarksModel::RootBookmark:
				{
					Action *action(new MenuAction(ActionsManager::PasteAction, {{QLatin1String("note"), bookmark->getIdentifier()}}, getExecutor(), this));
					action->setTextOverride(bookmark->getTitle(), false);

					if (bookmark->isFolder())
					{
						if (bookmark->hasChildren())
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

				break;
			default:
				addSeparator();

				break;
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

		Action *openUrlAction(new MenuAction(ActionsManager::OpenUrlAction, parameters, executor, this));
		openUrlAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Default Application"));

		addAction(openUrlAction);
	}
	else
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			const ApplicationInformation application(applications.at(i));

			parameters[QLatin1String("application")] = application.command;

			const bool hasValidName(!application.name.isEmpty());
			Action *openUrlAction(new MenuAction(ActionsManager::OpenUrlAction, parameters, executor, this));
			openUrlAction->setTextOverride((hasValidName ? application.name : QT_TRANSLATE_NOOP("actions", "Unknown")), !hasValidName);
			openUrlAction->setIconOverride(application.icon);

			addAction(new MenuAction(ActionsManager::OpenUrlAction, parameters, executor, this));

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
			Action *action(new MenuAction(definition.getTitle(), false, this));
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

void Menu::populateSearchEnginesMenu()
{
	clear();

	const QStringList searchEngines((m_role == ValidateMenu) ? SettingsManager::getOption(SettingsManager::Browser_ValidatorsOrderOption).toStringList() : SearchEnginesManager::getSearchEngines());
	const ActionExecutor::Object executor(getExecutor());

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

		if (searchEngine.isValid())
		{
			Action *action(new MenuAction(ActionsManager::SearchAction, {{QLatin1String("searchEngine"), searchEngine.identifier}, {QLatin1String("queryPlaceholder"), ((m_role == ValidateMenu) ? QLatin1String("{pageUrl}") : QLatin1String("{selection}"))}}, executor, this));
			action->setTextOverride(searchEngine.title, false);
			action->setIconOverride(searchEngine.icon);
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

	addAction(new MenuAction(ActionsManager::SaveSessionAction, {}, executor, this));
	addAction(new MenuAction(ActionsManager::SessionsAction, {}, executor, this));
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
		const SessionInformation session(sorted.at(i));
		int windows(0);

		for (int j = 0; j < session.windows.count(); ++j)
		{
			windows += session.windows.at(j).windows.count();
		}

		QAction *action(addAction(tr("%1 (%n tab(s))", "", windows).arg(session.title.isEmpty() ? tr("(Untitled)") : QString(session.title).replace(QLatin1Char('&'), QLatin1String("&&")))));
		action->setData(session.path);
		action->setCheckable(true);
		action->setChecked(session.path == currentSession);

		m_actionGroup->addAction(action);
	}
}

void Menu::populateSpellCheckSuggestionsMenu()
{
	clear();

	const MenuContext context(getMenuContext());

	if (!context.webWidget)
	{
		return;
	}

	const QString misspelledWord(context.webWidget->getMisspelledWord());
	const QStringList suggestions(context.webWidget->getSpellCheckerSuggestions());

	for (int i = 0; i < suggestions.count(); ++i)
	{
		const QString suggestion(suggestions.at(i));

		addAction(suggestion, context.webWidget, [=]()
		{
			context.webWidget->replaceMisspelledWord(suggestion);
		});
	}

	if (suggestions.isEmpty())
	{
		QAction *noSuggestionsAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "(no suggestions)"), true, this));
		noSuggestionsAction->setEnabled(false);

		addAction(noSuggestionsAction);
	}

	addSeparator();

	if (!misspelledWord.isEmpty() && SpellCheckManager::isIgnoringWord(misspelledWord))
	{
		Action *removeMisspelledWord(new MenuAction(QT_TRANSLATE_NOOP("actions", "Remove Word"), true, this));

		addAction(removeMisspelledWord);

		connect(removeMisspelledWord, &MenuAction::triggered, this, [=]()
		{
			SpellCheckManager::removeIgnoredWord(misspelledWord);
		});
	}
	else
	{
		Action *addMisspelledWord(new MenuAction(QT_TRANSLATE_NOOP("actions", "Add Word"), true, this));
		addMisspelledWord->setEnabled(!misspelledWord.isEmpty());

		addAction(addMisspelledWord);

		connect(addMisspelledWord, &MenuAction::triggered, this, [=]()
		{
			SpellCheckManager::addIgnoredWord(misspelledWord);
		});
	}
}

void Menu::populateStyleSheetsMenu()
{
	clear();

	const MenuContext context(getMenuContext());
	Action *defaultAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Default Style"), true, this));
	defaultAction->setData(-1);
	defaultAction->setCheckable(true);
	defaultAction->setChecked(true);

	addAction(defaultAction);

	if (!context.webWidget)
	{
		return;
	}

	addSeparator();

	const QString activeStyleSheet(context.webWidget->getActiveStyleSheet());
	const QStringList styleSheets(context.webWidget->getStyleSheets());
	QActionGroup *actionGroup(new QActionGroup(this));
	actionGroup->setExclusive(true);
	actionGroup->addAction(defaultAction);

	for (int i = 0; i < styleSheets.count(); ++i)
	{
		const QString styleSheet(styleSheets.at(i));
		QAction *action(addAction(styleSheet));
		action->setCheckable(true);
		action->setChecked(styleSheet == activeStyleSheet);

		actionGroup->addAction(action);
	}

	defaultAction->setChecked(activeStyleSheet.isEmpty() || !styleSheets.contains(activeStyleSheet));
}

void Menu::populateToolBarsMenu()
{
	clear();

	const ActionExecutor::Object executor(getExecutor());
	const QVector<ToolBarsManager::ToolBarDefinition> definitions(ToolBarsManager::getToolBarDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		addAction(new MenuAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), ToolBarsManager::getToolBarName(definitions.at(i).identifier)}}, executor, this));
	}

	addSeparator();

	Menu *addNewMenu(new Menu(UnknownMenu, this));
	Action *addNewAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Add New"), true, this));
	addNewAction->setMenu(addNewMenu);

	Action *addToolBarAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Add Toolbar…"), true, addNewMenu));
	Action *addBookmarksBarAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Add Bookmarks Bar…"), true, addNewMenu));
	Action *addSideBarAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Add Sidebar…"), true, addNewMenu));

	addNewMenu->addAction(addToolBarAction);
	addNewMenu->addAction(addBookmarksBarAction);
	addNewMenu->addAction(addSideBarAction);

	addAction(addNewAction);
	addSeparator();
	addAction(new MenuAction(ActionsManager::LockToolBarsAction, {}, executor, this));
	addSeparator();
	addAction(new MenuAction(ActionsManager::ResetToolBarsAction, {}, executor, this));

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
	const QString currentUserAgent(mainWindow ? mainWindow->getOption(SettingsManager::Network_UserAgentOption).toString() : QString());
	const QStringList userAgents(isRoot ? NetworkManagerFactory::getUserAgents() : NetworkManagerFactory::getUserAgent(menuAction()->data().toString()).children);

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const QString userAgent(userAgents.at(i));

		if (userAgent.isEmpty())
		{
			addSeparator();

			continue;
		}

		const UserAgentDefinition definition(NetworkManagerFactory::getUserAgent(userAgent));
		QAction *action(addAction(Utils::elideText(definition.getTitle(), fontMetrics(), this)));
		action->setData(userAgent);

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
			action->setChecked(userAgent == currentUserAgent);
		}

		m_actionGroup->addAction(action);
	}

	if (isRoot)
	{
		addSeparator();

		Action *customAction(new MenuAction(QT_TRANSLATE_NOOP("actions", "Custom User Agent…"), true, this));
		customAction->setData(QLatin1String("custom"));
		customAction->setCheckable(true);
		customAction->setChecked(currentUserAgent.startsWith(QLatin1String("custom;")));

		m_actionGroup->addAction(customAction);

		addAction(customAction);
	}
}

void Menu::populateWindowsMenu()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (isEmpty())
	{
		if (mainWindow)
		{
			connect(mainWindow, &MainWindow::titleChanged, this, &Menu::populateWindowsMenu);
			connect(mainWindow, &MainWindow::windowAdded, this, &Menu::populateWindowsMenu);
			connect(mainWindow, &MainWindow::windowRemoved, this, &Menu::populateWindowsMenu);
		}

		disconnect(this, &Menu::aboutToShow, this, &Menu::populateWindowsMenu);
	}

	clear();

	const MainWindowSessionItem *mainWindowItem(SessionsManager::getModel()->getMainWindowItem(mainWindow));

	if (!mainWindowItem)
	{
		return;
	}

	ActionExecutor::Object executor(mainWindow, mainWindow);

	for (int i = 0; i < mainWindowItem->rowCount(); ++i)
	{
		const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(mainWindowItem->child(i, 0)));

		if (windowItem)
		{
			const QVariantMap paramaters({{QLatin1String("tab"), windowItem->getActiveWindow()->getIdentifier()}});

			addAction(new MenuAction(ActionsManager::ActivateTabAction, paramaters, executor, this));
		}
	}
}

void Menu::clearBookmarksMenu()
{
	const int offset((m_menuOptions.value(QLatin1String("bookmark")).toULongLong() == 0) ? 3 : 0);

	for (int i = (actions().count() - 1); i >= offset; --i)
	{
		QAction *action(actions().at(i));
		action->deleteLater();

		removeAction(action);
	}

	connect(this, &Menu::aboutToShow, this, &Menu::populateBookmarksMenu);
}

void Menu::clearNotesMenu()
{
	clear();

	if (menuAction())
	{
		const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(m_menuOptions.value(QLatin1String("bookmark")).toULongLong()));

		menuAction()->setEnabled(bookmark && bookmark->hasChildren());
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

void Menu::setExecutor(const ActionExecutor::Object &executor)
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

Menu::MenuContext Menu::getMenuContext() const
{
	MenuContext context;
	context.mainWindow = MainWindow::findMainWindow(parentWidget());

	if (context.mainWindow)
	{
		context.window = context.mainWindow->getActiveWindow();

		if (context.window)
		{
			context.webWidget = context.window->getWebWidget();
		}
	}

	return context;
}

int Menu::getRole() const
{
	return m_role;
}

int Menu::getMenuRoleIdentifier(const QString &name)
{
	if (m_menuRoleIdentifierEnumerator < 0)
	{
		m_menuRoleIdentifierEnumerator = staticMetaObject.indexOfEnumerator(QLatin1String("MenuRole").data());
	}

	return EnumeratorMapper(staticMetaObject.enumerator(m_menuRoleIdentifierEnumerator), QLatin1String("Menu")).mapToValue(name, true);
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

MenuAction::MenuAction(const QString &text, bool isTranslateable, QMenu *parent) : Action(text, isTranslateable, parent),
	m_menu(parent)
{
}

MenuAction::MenuAction(int identifier, const QVariantMap &parameters, const ActionExecutor::Object &executor, QMenu *parent) : Action(identifier, parameters, executor, parent),
	m_menu(parent)
{
	switch (identifier)
	{
		case ActionsManager::PreferencesAction:
			setMenuRole(QAction::PreferencesRole);

			break;
		case ActionsManager::AboutQtAction:
			setMenuRole(QAction::AboutQtRole);

			break;
		case ActionsManager::ExitAction:
			setMenuRole(QAction::QuitRole);

			break;
		case ActionsManager::AboutApplicationAction:
			setMenuRole(QAction::AboutRole);

			break;
		default:
			break;
	}
}

void MenuAction::setState(const ActionsManager::ActionDefinition::State &state)
{
	Action::setState(state);

	int maximumWidth(-1);

	if (!shortcut().isEmpty())
	{
		const int shortcutWidth(m_menu->fontMetrics().boundingRect(QLatin1Char('X') + shortcut().toString(QKeySequence::NativeText)).width());
		const int availableWidth(m_menu->screen()->geometry().width() / 4);

		if (shortcutWidth < availableWidth)
		{
			maximumWidth = (availableWidth - shortcutWidth);
		}
	}

	setText(Utils::elideText(QString(state.text).replace(QLatin1Char('&'), QLatin1String("&&")), m_menu->fontMetrics(), m_menu, maximumWidth));
}

QMenu* MenuAction::createContextMenu(QWidget *parent) const
{
	Q_UNUSED(parent);

	return nullptr;
}

bool MenuAction::hasContextMenu() const
{
	return false;
}

OpenBookmarkMenuAction::OpenBookmarkMenuAction(quint64 bookmark, const ActionExecutor::Object &executor, QMenu *parent) : MenuAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark}}, executor, parent)
{
}

QMenu* OpenBookmarkMenuAction::createContextMenu(QWidget *parent) const
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent));
	Menu *menu(new Menu(Menu::BookmarkMenu, parent));
	menu->setExecutor(ActionExecutor::Object(mainWindow, mainWindow));
	menu->setMenuOptions({{QLatin1String("bookmark"), getParameters().value(QLatin1String("bookmark")).toULongLong()}});

	return menu;
}

bool OpenBookmarkMenuAction::hasContextMenu() const
{
	return true;
}

}
