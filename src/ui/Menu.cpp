/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	m_bookmark(nullptr),
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
		case BookmarksMenuRole:
		case BookmarkSelectorMenuRole:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Bookmarks"));
				installEventFilter(this);

				const Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
				{
					connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(clearBookmarksMenu()));
				}

				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateBookmarksMenu()));
			}

			break;
		case CharacterEncodingMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Character Encoding"));

			m_option = SettingsManager::Content_DefaultCharacterEncodingOption;

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateCharacterEncodingMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectOption(QAction*)));

			break;
		case ClosedWindowsMenuRole:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Closed Tabs and Windows"));
				setIcon(ThemesManager::createIcon(QLatin1String("user-trash")));

				const MainWindow *mainWindow(MainWindow::findMainWindow(parent));

				if (mainWindow)
				{
					setEnabled(!SessionsManager::getClosedWindows().isEmpty() || !mainWindow->getClosedWindows().isEmpty());

					connect(mainWindow, SIGNAL(closedWindowsAvailableChanged(bool)), this, SLOT(updateClosedWindowsMenu()));
				}

				connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindowsMenu()));
				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateClosedWindowsMenu()));
			}

			break;
		case DictionariesMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Dictionaries"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateDictionariesMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectDictionary(QAction*)));

			break;
		case ImportExportMenuRole:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Import and Export"));

				Action *importOperaBookmarksAction(new Action(-1, {}, this));
				importOperaBookmarksAction->setData(QLatin1String("OperaBookmarks"));
				importOperaBookmarksAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Bookmarks…"));

				Action *importHtmlBookmarksAction(new Action(-1, {}, this));
				importHtmlBookmarksAction->setData(QLatin1String("HtmlBookmarks"));
				importHtmlBookmarksAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import HTML Bookmarks…"));

				Action *importOperaNotesAction(new Action(-1, {}, this));
				importOperaNotesAction->setData(QLatin1String("OperaNotes"));
				importOperaNotesAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Notes…"));

				Action *importOperaSearchEnginesAction(new Action(-1, {}, this));
				importOperaSearchEnginesAction->setData(QLatin1String("OperaSearchEngines"));
				importOperaSearchEnginesAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Search Engines…"));

				Action *importOperaSessionAction(new Action(-1, {}, this));
				importOperaSessionAction->setData(QLatin1String("OperaSession"));
				importOperaSessionAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Session…"));

				addAction(importOperaBookmarksAction);
				addAction(importHtmlBookmarksAction);
				addSeparator();
				addAction(importOperaNotesAction);
				addSeparator();
				addAction(importOperaSearchEnginesAction);
				addSeparator();
				addAction(importOperaSessionAction);

				connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openImporter(QAction*)));
			}

			break;
		case NotesMenuRole:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Insert Note"));

				const Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
				{
					connect(NotesManager::getModel(), SIGNAL(modelModified()), this, SLOT(clearBookmarksMenu()));
				}

				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateNotesMenu()));
			}

			break;
		case OpenInApplicationMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Open with"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateOpenInApplicationMenu()));

			break;
		case ProxyMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Proxy"));

			m_option = SettingsManager::Network_ProxyOption;

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateProxiesMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectOption(QAction*)));

			break;
		case SearchMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Search Using"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateSearchMenu()));

			break;
		case SessionsMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Sessions"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateSessionsMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openSession(QAction*)));

			break;
		case StyleSheetsMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Style"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateStyleSheetsMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectStyleSheet(QAction*)));

			break;
		case ToolBarsMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Toolbars"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateToolBarsMenu()));

			break;
		case UserAgentMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "User Agent"));

			m_option = SettingsManager::Network_UserAgentOption;

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateUserAgentMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectOption(QAction*)));

			break;
		case ValidateMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Validate Using"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateSearchMenu()));

			break;
		case WindowsMenuRole:
			setTitle(QT_TRANSLATE_NOOP("actions", "Tabs and Windows"));

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateWindowsMenu()));

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
	if (m_role == BookmarksMenuRole && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton))
	{
		const QAction *action(actionAt(event->pos()));

		if (action && action == m_clickedAction && action->isEnabled() && action->data().type() == QVariant::ULongLong)
		{
			QWidget *menu(this);

			while (menu)
			{
				menu->close();
				menu = menu->parentWidget();

				if (!menu || !menu->inherits(QLatin1String("QMenu").data()))
				{
					break;
				}
			}

			const BookmarksItem *bookmark(BookmarksManager::getModel()->getBookmark(action->data().toULongLong()));

			if (bookmark)
			{
				Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::DefaultOpen, event->button(), event->modifiers()))}}, parentWidget());
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
	if (m_role == BookmarksMenuRole)
	{
		const QAction *action(actionAt(event->pos()));

		if (action && action->isEnabled() && action->data().type() == QVariant::ULongLong)
		{
			m_bookmark = BookmarksManager::getModel()->getBookmark(action->data().toULongLong());

			QMenu contextMenu(this);
			contextMenu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
			contextMenu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(SessionsManager::NewTabOpen);
			contextMenu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(static_cast<int>(SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen));
			contextMenu.addSeparator();
			contextMenu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(SessionsManager::NewWindowOpen);
			contextMenu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(static_cast<int>(SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen));
			contextMenu.exec(event->globalPos());

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

	if (m_role == NoMenuRole)
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

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateOptionMenu()));
	connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectOption(QAction*)));
}

void Menu::appendAction(const QJsonValue &definition, const QStringList &includeSections, ActionExecutor::Object executor)
{
	if (definition.isObject())
	{
		const QJsonObject object(definition.toObject());

		if (!canInclude(object, includeSections))
		{
			return;
		}

		const QString type(object.value(QLatin1String("type")).toString());
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

			Action *action(new Action(identifier, parameters, executor, this));

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

			if (object.contains(QLatin1String("icon")))
			{
				const QString data(object.value(QLatin1String("icon")).toString());

				if (data.startsWith(QLatin1String("data:image/")))
				{
					action->setOverrideIcon(QIcon(Utils::loadPixmapFromDataUri(data)));
				}
				else
				{
					action->setOverrideIcon(ThemesManager::createIcon(data));
				}
			}

			if (object.contains(QLatin1String("title")))
			{
				action->setOverrideText(object.value(QLatin1String("title")).toString());
			}

			addAction(action);
		}
		else if (type == QLatin1String("include"))
		{
			const QJsonArray actions(object.value(QLatin1String("actions")).toArray());

			for (int i = 0; i < actions.count(); ++i)
			{
				appendAction(actions.at(i), includeSections, executor);
			}
		}
		else if (type == QLatin1String("menu"))
		{
			const QVariantMap options(object.value(QLatin1String("options")).toVariant().toMap());
			Menu *menu(new Menu(getMenuRoleIdentifier(object.value(QLatin1String("identifier")).toString()), this));
			menu->setActionParameters(parameters);
			menu->setMenuOptions(options);

			if (object.contains(QLatin1String("actions")))
			{
				menu->load(object, includeSections, executor);
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
			const int role(rawIdentifier.endsWith(QLatin1String("Menu")) ? getMenuRoleIdentifier(rawIdentifier) : NoMenuRole);

			if (role == NoMenuRole)
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
				addMenu(new Menu(role, this));
			}
		}
	}
}

void Menu::populateBookmarksMenu()
{
	Menu *menu(qobject_cast<Menu*>(sender()));

	if (!menu || !menu->menuAction() || ((!menu->actions().isEmpty() && !(m_role == BookmarksMenuRole && menuAction()->data().toULongLong() == 0 && menu->actions().count() == 3))))
	{
		return;
	}

	const BookmarksModel *model(BookmarksManager::getModel());
	BookmarksItem *bookmark(model->getBookmark(menu->menuAction()->data().toULongLong()));

	if (!bookmark)
	{
		bookmark = model->getRootItem();
	}

	if (bookmark->rowCount() > 1 && m_role == BookmarksMenuRole)
	{
		Action *openAllAction(new Action(-1, {}, this));
		openAllAction->setData(bookmark->data(BookmarksModel::IdentifierRole).toULongLong());
		openAllAction->setIcon(ThemesManager::createIcon(QLatin1String("document-open-folder")));
		openAllAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open All"));

		menu->addAction(openAllAction);
		menu->addSeparator();

		connect(openAllAction, SIGNAL(triggered()), this, SLOT(openBookmark()));
	}
	else if (m_role == BookmarkSelectorMenuRole)
	{
		Action *addFolderAction(new Action(-1, {}, this));
		addFolderAction->setData(bookmark->data(BookmarksModel::IdentifierRole).toULongLong());
		addFolderAction->setIcon(ThemesManager::createIcon(QLatin1String("document-open-folder")));
		addFolderAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "This Folder"));

		menu->addAction(addFolderAction);
		menu->addSeparator();
	}

	for (int i = 0; i < bookmark->rowCount(); ++i)
	{
		const QModelIndex index(bookmark->index().child(i, 0));

		if (!index.isValid())
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark)
		{
			Action *action(new Action(-1, {}, this));
			action->setData(index.data(BookmarksModel::IdentifierRole));
			action->setOverrideIcon(index.data(Qt::DecorationRole).value<QIcon>());
			action->setToolTip(index.data(BookmarksModel::DescriptionRole).toString());
			action->setStatusTip(index.data(BookmarksModel::UrlRole).toString());

			if (index.data(BookmarksModel::TitleRole).toString().isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setOverrideText(Utils::elideText(QString(index.data(BookmarksModel::TitleRole).toString()).replace(QLatin1Char('&'), QLatin1String("&&")), menu));
			}

			if (type == BookmarksModel::UrlBookmark && m_role == BookmarksMenuRole)
			{
				connect(action, SIGNAL(triggered()), this, SLOT(openBookmark()));
			}
			else if (type == BookmarksModel::FolderBookmark)
			{
				if (model->rowCount(index) > 0)
				{
					action->setMenu(new Menu(m_role, this));
				}
				else
				{
					action->setEnabled(false);
				}
			}

			menu->addAction(action);
		}
		else
		{
			menu->addSeparator();
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

	m_actionGroup = new QActionGroup(this);
	m_actionGroup->setExclusive(true);

	const QVector<SettingsManager::OptionDefinition::ChoiceDefinition> choices(SettingsManager::getOptionDefinition(m_option).choices);

	if (choices.isEmpty())
	{
		return;
	}

	for (int i = 0; i < choices.count(); ++i)
	{
		Action *action(new Action(-1, {}, this));
		action->setCheckable(true);
		action->setChecked(choices.at(i).value == value);
		action->setData(choices.at(i).value);
		action->setIcon(choices.at(i).icon);

		m_actionGroup->addAction(action);

		if (choices.at(i).value == QLatin1String("ask"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Ask What to Do"));
		}
		else if (choices.at(i).value == QLatin1String("allow"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Always Allow"));
		}
		else if (choices.at(i).value == QLatin1String("disallow"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Always Deny"));
		}
		else if (choices.at(i).value == QLatin1String("keepUntilExpires"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Expires"));
		}
		else if (choices.at(i).value == QLatin1String("keepUntilExit"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Current Session is Closed"));
		}
		else if (choices.at(i).value == QLatin1String("acceptAll"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Always"));
		}
		else if (choices.at(i).value == QLatin1String("acceptExisting"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Only Existing"));
		}
		else if (choices.at(i).value == QLatin1String("readOnly"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Only Read Existing"));
		}
		else if (choices.at(i).value == QLatin1String("ignore"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Ignore"));
		}
		else if (choices.at(i).value == QLatin1String("openAll"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open All"));
		}
		else if (choices.at(i).value == QLatin1String("openAllInBackground"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open in Background"));
		}
		else if (choices.at(i).value == QLatin1String("blockAll"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Block All"));
		}
		else if (choices.at(i).value == QLatin1String("onlyCached"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Only Cached"));
		}
		else if (choices.at(i).value == QLatin1String("enabled"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Enabled"));
		}
		else if (choices.at(i).value == QLatin1String("onDemand"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "On Demand"));
		}
		else if (choices.at(i).value == QLatin1String("disabled"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Disabled"));
		}
		else
		{
			action->setOverrideText(choices.at(i).getTitle());
		}

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

		Action *defaultAction(new Action(-1, {}, this));
		defaultAction->setData(QLatin1String("auto"));
		defaultAction->setCheckable(true);
		defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Auto Detect"));

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

			QAction *textCodecAction(addAction(Utils::elideText(codec->name(), this)));
			textCodecAction->setData(codec->name().toLower());
			textCodecAction->setCheckable(true);

			m_actionGroup->addAction(textCodecAction);
		}
	}

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString encoding(mainWindow ? mainWindow->getOption(SettingsManager::Content_DefaultCharacterEncodingOption).toString().toLower() : QString());

	for (int i = 2; i < actions().count(); ++i)
	{
		QAction *action(actions().at(i));

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

	Action *clearAction(new Action(-1, {}, this));
	clearAction->setEnabled(false);
	clearAction->setIcon(ThemesManager::createIcon(QLatin1String("edit-clear")));
	clearAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Clear"));

	addAction(clearAction);
	addSeparator();

	const QStringList windows(SessionsManager::getClosedWindows());

	if (!windows.isEmpty())
	{
		ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());

		for (int i = 0; i < windows.count(); ++i)
		{
			Action *action((i == 0) ? new Action(ActionsManager::ReopenWindowAction, {}, executor, this) : new Action(ActionsManager::ReopenWindowAction, {{QLatin1String("index"), i}}, executor, this));
			action->setOverrideText(Utils::elideText(tr("Window - %1").arg(windows.at(i)), this));

			addAction(action);
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
				Action *action((i == 0) ? new Action(ActionsManager::ReopenTabAction, {}, executor, this) : new Action(ActionsManager::ReopenTabAction, {{QLatin1String("index"), i}}, executor, this));
				action->setOverrideIcon(tabs.at(i).isPrivate ? ThemesManager::createIcon(QLatin1String("tab-private")) : tabs.at(i).icon);
				action->setOverrideText(Utils::elideText(tabs.at(i).window.getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), this));
				action->setStatusTip(tabs.at(i).window.getUrl());

				addAction(action);
			}

			clearAction->setEnabled(true);
		}
	}

	connect(clearAction, SIGNAL(triggered()), this, SLOT(clearClosedWindows()));
}

void Menu::populateDictionariesMenu()
{
	clear();

	const MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getActiveWindow());

	if (!window || !window->getContentsWidget()->getWebWidget())
	{
		return;
	}

	QString dictionary(window->getContentsWidget()->getWebWidget()->getOption(SettingsManager::Browser_SpellCheckDictionaryOption).toString());

	if (dictionary.isEmpty())
	{
		dictionary = SpellCheckManager::getDefaultDictionary();
	}

	const QVector<SpellCheckManager::DictionaryInformation> dictionaries(window->getContentsWidget()->getWebWidget()->getDictionaries());
	QActionGroup *actionGroup(new QActionGroup(this));
	actionGroup->setExclusive(true);

	for (int i = 0; i < dictionaries.count(); ++i)
	{
		QAction *action(addAction(dictionaries.at(i).title));
		action->setCheckable(true);
		action->setChecked(dictionaries.at(i).name == dictionary);
		action->setData(dictionaries.at(i).name);

		actionGroup->addAction(action);
	}
}

void Menu::populateNotesMenu()
{
	Menu *menu(qobject_cast<Menu*>(sender()));

	if (!menu || !menu->menuAction() || !menu->actions().isEmpty())
	{
		return;
	}

	const BookmarksModel *model(NotesManager::getModel());
	BookmarksItem *bookmark(model->getBookmark(menu->menuAction()->data().toULongLong()));

	if (!bookmark)
	{
		bookmark = model->getRootItem();
	}

	for (int i = 0; i < bookmark->rowCount(); ++i)
	{
		const QModelIndex index(bookmark->index().child(i, 0));

		if (!index.isValid())
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark)
		{
			Action *action(new Action(ActionsManager::PasteAction, {{QLatin1String("note"), index.data(BookmarksModel::IdentifierRole)}}, getExecutor(), this));
			action->setOverrideIcon(index.data(Qt::DecorationRole).value<QIcon>());
			action->setToolTip(index.data(BookmarksModel::DescriptionRole).toString());
			action->setStatusTip(index.data(BookmarksModel::UrlRole).toString());

			if (index.data(BookmarksModel::TitleRole).toString().isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setOverrideText(Utils::elideText(QString(index.data(BookmarksModel::TitleRole).toString()).replace(QLatin1Char('&'), QLatin1String("&&")), menu));
			}

			if (type == BookmarksModel::FolderBookmark)
			{
				if (model->rowCount(index) > 0)
				{
					action->setMenu(new Menu(m_role, this));
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
			menu->addSeparator();
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

		Action *action(new Action(ActionsManager::OpenUrlAction, parameters, executor, this));
		action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Default Application"));

		addAction(action);
	}
	else
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			parameters[QLatin1String("application")] = applications.at(i).command;

			Action *action(new Action(ActionsManager::OpenUrlAction, parameters, executor, this));
			action->setOverrideIcon(applications.at(i).icon);
			action->setOverrideText(((applications.at(i).name.isEmpty()) ? QT_TRANSLATE_NOOP("actions", "Unknown") : applications.at(i).name));

			addAction(action);

			if (i == 0)
			{
				addSeparator();
			}
		}
	}

	disconnect(this, SIGNAL(aboutToShow()), this, SLOT(populateOpenInApplicationMenu()));
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
			Action *action(new Action(-1, {}, this));
			action->setData(proxies.at(i));
			action->setText(Utils::elideText(definition.getTitle(), this));

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

	const QStringList searchEngines((m_role == ValidateMenuRole) ? SettingsManager::getOption(SettingsManager::Browser_ValidatorsOrderOption).toStringList() : SearchEnginesManager::getSearchEngines());
	ActionExecutor::Object executor(getExecutor());

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

		if (searchEngine.isValid())
		{
			Action *action(new Action(ActionsManager::SearchAction, {{QLatin1String("searchEngine"), searchEngine.identifier}, {QLatin1String("queryPlaceholder"), ((m_role == ValidateMenuRole) ? QLatin1String("{pageUrl}") : QLatin1String("{selection}"))}}, executor, this));
			action->setOverrideIcon(searchEngine.icon);
			action->setOverrideText(searchEngine.title);
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
	Action *defaultAction(new Action(-1, {}, this));
	defaultAction->setData(-1);
	defaultAction->setCheckable(true);
	defaultAction->setChecked(true);
	defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Default Style"));

	addAction(defaultAction);

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getActiveWindow());

	if (!window || !window->getContentsWidget()->getWebWidget())
	{
		return;
	}

	addSeparator();

	const QString activeStyleSheet(window->getContentsWidget()->getWebWidget()->getActiveStyleSheet());
	const QStringList styleSheets(window->getContentsWidget()->getWebWidget()->getStyleSheets());
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

	Menu *addNewMenu(new Menu(NoMenuRole, this));
	Action *addNewAction(new Action(-1, {}, this));
	addNewAction->setMenu(addNewMenu);
	addNewAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add New"));

	Action *addToolBarAction(new Action(-1, {}, addNewMenu));
	addToolBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Toolbar…"));

	Action *addBookmarksBarAction(new Action(-1, {}, addNewMenu));
	addBookmarksBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Bookmarks Bar…"));

	Action *addSideBarAction(new Action(-1, {}, addNewMenu));
	addSideBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Sidebar…"));

	addNewMenu->addAction(addToolBarAction);
	addNewMenu->addAction(addBookmarksBarAction);
	addNewMenu->addAction(addSideBarAction);

	addAction(addNewAction);
	addSeparator();
	addAction(new Action(ActionsManager::LockToolBarsAction, {}, executor, this));
	addSeparator();
	addAction(new Action(ActionsManager::ResetToolBarsAction, {}, executor, this));

	connect(addToolBarAction, SIGNAL(triggered()), ToolBarsManager::getInstance(), SLOT(addToolBar()));
	connect(addBookmarksBarAction, SIGNAL(triggered()), ToolBarsManager::getInstance(), SLOT(addBookmarksBar()));
	connect(addSideBarAction, SIGNAL(triggered()), ToolBarsManager::getInstance(), SLOT(addSideBar()));
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
			QAction *action(addAction(Utils::elideText(definition.getTitle(), this)));
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

		Action *customAction(new Action(-1, {}, this));
		customAction->setData(QLatin1String("custom"));
		customAction->setCheckable(true);
		customAction->setChecked(userAgent.startsWith(QLatin1String("custom;")));
		customAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Custom User Agent…"));

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
			connect(mainWindow, SIGNAL(titleChanged(QString)), this, SLOT(populateWindowsMenu()));
			connect(mainWindow, SIGNAL(windowAdded(quint64)), this, SLOT(populateWindowsMenu()));
			connect(mainWindow, SIGNAL(windowRemoved(quint64)), this, SLOT(populateWindowsMenu()));
		}

		disconnect(this, SIGNAL(aboutToShow()), this, SLOT(populateWindowsMenu()));
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
				Action *action(new Action(ActionsManager::ActivateTabAction, {{QLatin1String("tab"), windowItem->getActiveWindow()->getIdentifier()}}, executor, this));
				action->setOverrideIcon(windowItem->getActiveWindow()->getIcon());
				action->setOverrideText(Utils::elideText((windowItem->getActiveWindow()->getTitle().isEmpty() ? QT_TRANSLATE_NOOP("actions", "(Untitled)") : windowItem->getActiveWindow()->getTitle()), this));

				addAction(action);
			}
		}
	}
}

void Menu::clearBookmarksMenu()
{
	const int offset((menuAction() && menuAction()->data().toULongLong() == 0) ? 3 : 0);

	for (int i = (actions().count() - 1); i >= offset; --i)
	{
		actions().at(i)->deleteLater();

		removeAction(actions().at(i));
	}

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateBookmarksMenu()));
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
		const BookmarksItem *bookmark(NotesManager::getModel()->getBookmark(menuAction()->data().toULongLong()));

		menuAction()->setEnabled(bookmark && bookmark->rowCount() > 0);
	}

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateNotesMenu()));
}

void Menu::openBookmark()
{
	QWidget *menu(this);

	while (menu)
	{
		menu->close();
		menu = menu->parentWidget();

		if (!menu || !menu->inherits(QLatin1String("QMenu").data()))
		{
			break;
		}
	}

	const QAction *action(qobject_cast<QAction*>(sender()));

	if (action && action->data().type() == QVariant::ULongLong)
	{
		m_bookmark = BookmarksManager::getModel()->getBookmark(action->data().toULongLong());
	}

	const SessionsManager::OpenHints hints((action && action->data().type() != QVariant::ULongLong) ? static_cast<SessionsManager::OpenHints>(action->data().toInt()) : SessionsManager::DefaultOpen);

	Application::triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), m_bookmark->data(BookmarksModel::IdentifierRole)}, {QLatin1String("hints"), QVariant((hints == SessionsManager::DefaultOpen) ? SessionsManager::calculateOpenHints() : hints)}}, parentWidget());

	m_bookmark = nullptr;
}

void Menu::openImporter(QAction *action)
{
	if (action)
	{
		ImportDialog::createDialog(action->data().toString(), MainWindow::findMainWindow(this));
	}
}

void Menu::openSession(QAction *action)
{
	if (!action->data().isNull())
	{
		SessionsManager::restoreSession(SessionsManager::getSession(action->data().toString()), (SettingsManager::getOption(SettingsManager::Sessions_OpenInExistingWindowOption).toBool() ? Application::getActiveWindow() : nullptr));
	}
}

void Menu::selectDictionary(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getActiveWindow());

	if (window && window->getContentsWidget()->getWebWidget() && action)
	{
		window->getContentsWidget()->getWebWidget()->setOption(SettingsManager::Browser_SpellCheckDictionaryOption, action->data().toString());
	}
}

void Menu::selectOption(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		mainWindow->setOption(m_option, action->data().toString());
	}
}

void Menu::selectStyleSheet(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getActiveWindow());

	if (window && window->getContentsWidget()->getWebWidget() && action)
	{
		window->getContentsWidget()->getWebWidget()->setActiveStyleSheet(action->data().isNull() ? action->text() : QString());
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

ActionExecutor::Object Menu::getExecutor()
{
	if (m_executor.isValid())
	{
		return m_executor;
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(this));

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

	if (!name.endsWith(QLatin1String("Role")))
	{
		return Menu::staticMetaObject.enumerator(m_menuRoleIdentifierEnumerator).keyToValue(QString(name + QLatin1String("Role")).toLatin1());
	}

	return Menu::staticMetaObject.enumerator(m_menuRoleIdentifierEnumerator).keyToValue(name.toLatin1());
}

bool Menu::canInclude(const QJsonObject &definition, const QStringList &includeSections)
{
	if (definition.contains(QLatin1String("excludeFrom")) && includeSections.contains(definition.value(QLatin1String("excludeFrom")).toString()))
	{
		return false;
	}

	if (definition.contains(QLatin1String("includeIn")) && !includeSections.contains(definition.value(QLatin1String("includeIn")).toString()))
	{
		return false;
	}

	return true;
}

}
