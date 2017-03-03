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
#include "ContentsWidget.h"
#include "ImportDialog.h"
#include "MainWindow.h"
#include "Window.h"
#include "../core/ActionsManager.h"
#include "../core/BookmarksManager.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/NotesManager.h"
#include "../core/SearchEnginesManager.h"
#include "../core/SessionsManager.h"
#include "../core/ThemesManager.h"
#include "../core/ToolBarsManager.h"
#include "../core/Utils.h"
#include "../core/WindowsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTextCodec>
#include <QtGui/QMouseEvent>

namespace Otter
{

Menu::Menu(MenuRole role, QWidget *parent) : QMenu(parent),
	m_actionGroup(nullptr),
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
		case NotesMenuRole:
			{
				setTitle((role == NotesMenuRole) ? QT_TRANSLATE_NOOP("actions", "Notes") : QT_TRANSLATE_NOOP("actions", "Bookmarks"));
				installEventFilter(this);

				Menu *parentMenu(qobject_cast<Menu*>(parent));

				if (!parentMenu || parentMenu->getRole() != m_role)
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
			setTitle(QT_TRANSLATE_NOOP("actions", "Character Encoding"));

			m_option = SettingsManager::Content_DefaultCharacterEncodingOption;

			connect(this, SIGNAL(aboutToShow()), this, SLOT(populateCharacterEncodingMenu()));
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectOption(QAction*)));

			break;
		case ClosedWindowsMenu:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Closed Tabs and Windows"));
				setIcon(ThemesManager::getIcon(QLatin1String("user-trash")));

				MainWindow *mainWindow(MainWindow::findMainWindow(parent));

				if (mainWindow)
				{
					setEnabled(!SessionsManager::getClosedWindows().isEmpty() || !mainWindow->getWindowsManager()->getClosedWindows().isEmpty());

					connect(mainWindow->getWindowsManager(), SIGNAL(closedWindowsAvailableChanged(bool)), this, SLOT(updateClosedWindowsMenu()));
				}

				connect(SessionsManager::getInstance(), SIGNAL(closedWindowsChanged()), this, SLOT(updateClosedWindowsMenu()));
				connect(this, SIGNAL(aboutToShow()), this, SLOT(populateClosedWindowsMenu()));
			}

			break;
		case ImportExportMenuRole:
			{
				setTitle(QT_TRANSLATE_NOOP("actions", "Import and Export"));

				Action *importOperaBookmarksAction(addAction());
				importOperaBookmarksAction->setData(QLatin1String("OperaBookmarks"));
				importOperaBookmarksAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Bookmarks…"));

				Action *importHtmlBookmarksAction(addAction());
				importHtmlBookmarksAction->setData(QLatin1String("HtmlBookmarks"));
				importHtmlBookmarksAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import HTML Bookmarks…"));

				addSeparator();

				Action *importOperaNotesAction(addAction());
				importOperaNotesAction->setData(QLatin1String("OperaNotes"));
				importOperaNotesAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Notes…"));

				addSeparator();

				Action *importOperaSearchEnginesAction(addAction());
				importOperaSearchEnginesAction->setData(QLatin1String("OperaSearchEngines"));
				importOperaSearchEnginesAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Search Engines…"));

				addSeparator();

				Action *importOperaSessionAction(addAction());
				importOperaSessionAction->setData(QLatin1String("OperaSession"));
				importOperaSessionAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Import Opera Session…"));

				connect(this, SIGNAL(triggered(QAction*)), this, SLOT(openImporter(QAction*)));
			}

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
			connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectWindow(QAction*)));

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
		setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));
	}
}

void Menu::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_role == BookmarksMenuRole && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton))
	{
		QAction *action(actionAt(event->pos()));

		if (action && action->isEnabled() && action->data().type() == QVariant::ModelIndex)
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

			MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

			if (mainWindow)
			{
				mainWindow->getWindowsManager()->open(BookmarksManager::getModel()->getBookmark(action->data().toModelIndex()), WindowsManager::calculateOpenHints(WindowsManager::DefaultOpen, event->button(), event->modifiers()));

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
		QAction *action(actionAt(event->pos()));

		if (action && action->isEnabled() && action->data().type() == QVariant::ModelIndex)
		{
			m_bookmark = BookmarksManager::getModel()->getBookmark(action->data().toModelIndex());

			QMenu contextMenu(this);
			contextMenu.addAction(ThemesManager::getIcon(QLatin1String("document-open")), tr("Open"), this, SLOT(openBookmark()));
			contextMenu.addAction(tr("Open in New Tab"), this, SLOT(openBookmark()))->setData(WindowsManager::NewTabOpen);
			contextMenu.addAction(tr("Open in New Background Tab"), this, SLOT(openBookmark()))->setData(static_cast<int>(WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen));
			contextMenu.addSeparator();
			contextMenu.addAction(tr("Open in New Window"), this, SLOT(openBookmark()))->setData(WindowsManager::NewWindowOpen);
			contextMenu.addAction(tr("Open in New Background Window"), this, SLOT(openBookmark()))->setData(static_cast<int>(WindowsManager::NewWindowOpen | WindowsManager::BackgroundOpen));
			contextMenu.exec(event->globalPos());

			return;
		}
	}

	QMenu::contextMenuEvent(event);
}

void Menu::load(const QString &path, const QStringList &options)
{
	QFile file(SessionsManager::getReadableDataPath(path));
	file.open(QIODevice::ReadOnly);

	load(QJsonDocument::fromJson(file.readAll()).object(), options);

	file.close();
}

void Menu::load(const QJsonObject &definition, const QStringList &options)
{
	const QString identifier(definition.value(QLatin1String("identifier")).toString());

	if (m_role == NoMenuRole)
	{
		clear();
	}

	setObjectName(identifier);
	setTitle(definition.value(QLatin1String("title")).toString());

	const QJsonArray actions(definition.value(QLatin1String("actions")).toArray());

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i).isObject())
		{
			const QJsonObject object(actions.at(i).toObject());
			const QString type(object.value(QLatin1String("type")).toString());

			if (type == QLatin1String("action"))
			{
				const QVariantMap parameters(object.value(QLatin1String("parameters")).toVariant().toMap());
				const int identifier(ActionsManager::getActionIdentifier(object.value(QLatin1String("identifier")).toString()));

				if (identifier >= 0)
				{
					const QString text(object.value(QLatin1String("title")).toString());
					MainWindow *mainWindow(MainWindow::findMainWindow(this));
					Action *action(addAction(identifier, false));
					action->setParameters(parameters);

					if (!text.isEmpty())
					{
						action->setOverrideText(text);
					}

					if (object.contains(QLatin1String("icon")))
					{
						const QString data(object.value(QLatin1String("icon")).toString());

						if (data.startsWith(QLatin1String("data:image/")))
						{
							action->setIcon(QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8())))));
						}
						else
						{
							action->setIcon(ThemesManager::getIcon(data));
						}
					}

					if (mainWindow)
					{
						connect(action, SIGNAL(triggered(bool)), mainWindow, SLOT(triggerAction(bool)));
					}
				}

				continue;
			}

			if (object.contains(QLatin1String("excludeFrom")) && options.contains(object.value(QLatin1String("excludeFrom")).toString()))
			{
				continue;
			}

			if (object.contains(QLatin1String("includeIn")) && !options.contains(object.value(QLatin1String("includeIn")).toString()))
			{
				continue;
			}

			Menu *menu(new Menu(Menu::getRole(object.value(QLatin1String("identifier")).toString()), this));
			menu->load(object, options);

			if (type == QLatin1String("menu"))
			{
				addMenu(menu);
			}
			else if (type == QLatin1String("include"))
			{
				for (int j = 0; j < menu->actions().count(); ++j)
				{
					QMenu::addAction(menu->actions().at(j));
				}
			}
		}
		else
		{
			const QString rawIdentifier(actions.at(i).toString());
			const Menu::MenuRole role(rawIdentifier.endsWith(QLatin1String("Menu")) ? Menu::getRole(rawIdentifier) : Menu::NoMenuRole);

			if (rawIdentifier == QLatin1String("separator"))
			{
				addSeparator();
			}
			else if (role != Menu::NoMenuRole)
			{
				addMenu(new Menu(role, this));
			}
			else
			{
				const int identifier(ActionsManager::getActionIdentifier(rawIdentifier));

				if (identifier >= 0)
				{
					MainWindow *mainWindow(MainWindow::findMainWindow(this));

					if (mainWindow && Action::isLocal(identifier) && mainWindow->getWindowsManager()->getAction(identifier))
					{
						QMenu::addAction(mainWindow->getWindowsManager()->getAction(identifier));
					}
					else
					{
						addAction(identifier, true);
					}
				}
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

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateOptionMenu()));
	connect(this, SIGNAL(triggered(QAction*)), this, SLOT(selectOption(QAction*)));
}

void Menu::populateModelMenu()
{
	Menu *menu(qobject_cast<Menu*>(sender()));

	if (!menu || !menu->menuAction())
	{
		return;
	}

	QModelIndex index(menu->menuAction()->data().toModelIndex());

	if (!menu->actions().isEmpty() && !(m_role == BookmarksMenuRole && !index.isValid() && menu->actions().count() == 3))
	{
		return;
	}

	if (!index.isValid())
	{
		index = ((m_role == NotesMenuRole) ? NotesManager::getModel() : BookmarksManager::getModel())->getRootItem()->index();
	}

	const QAbstractItemModel *model(index.model());

	if (!model)
	{
		return;
	}

	if (model->rowCount(index) > 1 && m_role == BookmarksMenuRole)
	{
		Action *openAllAction(menu->addAction());
		openAllAction->setData(index);
		openAllAction->setIcon(ThemesManager::getIcon(QLatin1String("document-open-folder")));
		openAllAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open All"));

		menu->addSeparator();

		connect(openAllAction, SIGNAL(triggered()), this, SLOT(openBookmark()));
	}

	if (m_role == BookmarkSelectorMenuRole)
	{
		Action *addFolderAction(menu->addAction());
		addFolderAction->setData(index);
		addFolderAction->setIcon(ThemesManager::getIcon(QLatin1String("document-open-folder")));
		addFolderAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "This Folder"));

		menu->addSeparator();
	}

	for (int i = 0; i < model->rowCount(index); ++i)
	{
		const QModelIndex childIndex(index.child(i, 0));

		if (!childIndex.isValid())
		{
			continue;
		}

		const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(childIndex.data(BookmarksModel::TypeRole).toInt()));

		if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark || type == BookmarksModel::UrlBookmark)
		{
			Action *action(menu->addAction());
			action->setData(childIndex);
			action->setIcon(childIndex.data(Qt::DecorationRole).value<QIcon>());
			action->setToolTip(childIndex.data(BookmarksModel::DescriptionRole).toString());
			action->setStatusTip(childIndex.data(BookmarksModel::UrlRole).toString());

			if (childIndex.data(BookmarksModel::TitleRole).toString().isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setText(Utils::elideText(QString(childIndex.data(BookmarksModel::TitleRole).toString()).replace(QLatin1Char('&'), QLatin1String("&&")), menu));
			}

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

void Menu::populateOptionMenu()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString value(mainWindow ? mainWindow->getWindowsManager()->getOption(m_option).toString() : QString());

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

	const QStringList choices(SettingsManager::getOptionDefinition(m_option).choices);

	if (choices.isEmpty())
	{
		return;
	}

	for (int i = 0; i < choices.count(); ++i)
	{
		Action *action(addAction());
		action->setCheckable(true);
		action->setChecked(choices.at(i) == value);
		action->setData(choices.at(i));

		m_actionGroup->addAction(action);

		if (choices.at(i) == QLatin1String("ask"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Ask What to Do"));
		}
		else if (choices.at(i) == QLatin1String("allow"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Always Allow"));
		}
		else if (choices.at(i) == QLatin1String("disallow"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Always Deny"));
		}
		else if (choices.at(i) == QLatin1String("keepUntilExpires"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Expires"));
		}
		else if (choices.at(i) == QLatin1String("keepUntilExit"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Current Session is Closed"));
		}
		else if (choices.at(i) == QLatin1String("acceptAll"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Always"));
		}
		else if (choices.at(i) == QLatin1String("acceptExisting"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Only Existing"));
		}
		else if (choices.at(i) == QLatin1String("readOnly"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Only Read Existing"));
		}
		else if (choices.at(i) == QLatin1String("ignore"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Ignore"));
		}
		else if (choices.at(i) == QLatin1String("openAll"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open All"));
		}
		else if (choices.at(i) == QLatin1String("openAllInBackground"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Open in Background"));
		}
		else if (choices.at(i) == QLatin1String("blockAll"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Block All"));
		}
		else if (choices.at(i) == QLatin1String("onlyCached"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Only Cached"));
		}
		else if (choices.at(i) == QLatin1String("enabled"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Enabled"));
		}
		else if (choices.at(i) == QLatin1String("onDemand"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "On Demand"));
		}
		else if (choices.at(i) == QLatin1String("disabled"))
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Disabled"));
		}
		else
		{
			action->setOverrideText(choices.at(i));
		}
	}
}

void Menu::populateCharacterEncodingMenu()
{
	if (!m_actionGroup)
	{
		const QList<int> textCodecs({106, 1015, 1017, 4, 5, 6, 7, 8, 82, 10, 85, 12, 13, 109, 110, 112, 2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 18, 39, 17, 38, 2026});

		m_actionGroup = new QActionGroup(this);
		m_actionGroup->setExclusive(true);

		Action *defaultAction(addAction());
		defaultAction->setData(QLatin1String("auto"));
		defaultAction->setCheckable(true);
		defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Auto Detect"));

		m_actionGroup->addAction(defaultAction);

		addSeparator();

		for (int i = 0; i < textCodecs.count(); ++i)
		{
			QTextCodec *codec(QTextCodec::codecForMib(textCodecs.at(i)));

			if (!codec)
			{
				continue;
			}

			QAction *textCodecAction(QMenu::addAction(Utils::elideText(codec->name(), this)));
			textCodecAction->setData(codec->name().toLower());
			textCodecAction->setCheckable(true);

			m_actionGroup->addAction(textCodecAction);
		}
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString encoding(mainWindow ? mainWindow->getWindowsManager()->getOption(SettingsManager::Content_DefaultCharacterEncodingOption).toString().toLower() : QString());

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

	Action *clearAction(addAction());
	clearAction->setData(0);
	clearAction->setIcon(ThemesManager::getIcon(QLatin1String("edit-clear")));
	clearAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Clear"));

	addSeparator();

	const QStringList windows(SessionsManager::getClosedWindows());

	if (!windows.isEmpty())
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			QMenu::addAction(Utils::elideText(tr("Window - %1").arg(windows.at(i)), this), this, SLOT(restoreClosedWindow()))->setData(-(i + 1));
		}

		addSeparator();
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		const QList<ClosedWindow> tabs(mainWindow->getWindowsManager()->getClosedWindows());

		for (int i = 0; i < tabs.count(); ++i)
		{
			QAction *action(QMenu::addAction(HistoryManager::getIcon(QUrl(tabs.at(i).window.getUrl())), Utils::elideText(tabs.at(i).window.getTitle().replace(QLatin1Char('&'), QLatin1String("&&")), this), this, SLOT(restoreClosedWindow())));
			action->setData(i + 1);
			action->setStatusTip(tabs.at(i).window.getUrl());
		}
	}

	connect(clearAction, SIGNAL(triggered()), this, SLOT(clearClosedWindows()));
}

void Menu::populateProxiesMenu()
{
	if (m_actionGroup)
	{
		m_actionGroup->deleteLater();

		clear();
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString proxy(mainWindow ? mainWindow->getWindowsManager()->getOption(SettingsManager::Network_ProxyOption).toString() : QString());
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
			Action *action(addAction());
			action->setData(proxies.at(i));
			action->setText(Utils::elideText(definition.getTitle(), this));

			if (definition.isFolder)
			{
				action->setIcon(ThemesManager::getIcon(QLatin1String("inode-directory")));

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
		}
	}
}

void Menu::populateSearchMenu()
{
	clear();

	const QStringList searchEngines((m_role == ValidateMenuRole) ? SettingsManager::getOption(SettingsManager::Browser_ValidatorsOrderOption).toStringList() : SearchEnginesManager::getSearchEngines());
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(searchEngines.at(i)));

		if (!searchEngine.identifier.isEmpty())
		{
			QVariantMap parameters;
			parameters[QLatin1String("searchEngine")] = searchEngine.identifier;

			if (m_role == ValidateMenuRole)
			{
				parameters[QLatin1String("query")] = QLatin1String("{url}");
			}

			Action *action(addAction(ActionsManager::SearchAction));
			action->setParameters(parameters);
			action->setIcon(searchEngine.icon);
			action->setOverrideText(searchEngine.title);
			action->setToolTip(searchEngine.description);

			if (mainWindow)
			{
				connect(action, SIGNAL(triggered(bool)), mainWindow, SLOT(triggerAction(bool)));
			}
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

	addAction(ActionsManager::SaveSessionAction, true);
	addAction(ActionsManager::SessionsAction, true);

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

		QAction *action(QMenu::addAction(tr("%1 (%n tab(s))", "", windows).arg(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : QString(sorted.at(i).title).replace(QLatin1Char('&'), QLatin1String("&&")))));
		action->setData(sorted.at(i).path);
		action->setCheckable(true);
		action->setChecked(sorted.at(i).path == currentSession);

		m_actionGroup->addAction(action);
	}
}

void Menu::populateStyleSheetsMenu()
{
	clear();

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	Action *defaultAction(addAction());
	defaultAction->setData(-1);
	defaultAction->setCheckable(true);
	defaultAction->setChecked(true);
	defaultAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Default Style"));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getWindowsManager()->getWindowByIndex(-1));

	if (!window)
	{
		return;
	}

	addSeparator();

	const QString activeStyleSheet(window->getContentsWidget()->getActiveStyleSheet());
	const QStringList styleSheets(window->getContentsWidget()->getStyleSheets());
	QActionGroup *actionGroup(new QActionGroup(this));
	actionGroup->setExclusive(true);
	actionGroup->addAction(defaultAction);

	for (int i = 0; i < styleSheets.count(); ++i)
	{
		QAction *action(QMenu::addAction(styleSheets.at(i)));

		action->setCheckable(true);
		action->setChecked(styleSheets.at(i) == activeStyleSheet);

		actionGroup->addAction(action);
	}

	defaultAction->setChecked(activeStyleSheet.isEmpty() || !styleSheets.contains(activeStyleSheet));
}

void Menu::populateToolBarsMenu()
{
	clear();

	const QVector<ToolBarsManager::ToolBarDefinition> definitions(ToolBarsManager::getToolBarDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		Action *action(addAction());
		action->setData(definitions.at(i).identifier);
		action->setCheckable(true);
		action->setChecked(definitions.at(i).normalVisibility != ToolBarsManager::AlwaysHiddenToolBar);

		if (definitions.at(i).getTitle().isEmpty())
		{
			action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
		}
		else
		{
			action->setText(definitions.at(i).getTitle());
		}

		connect(action, SIGNAL(toggled(bool)), this, SLOT(setToolBarVisibility(bool)));
	}

	addAction(ActionsManager::ShowErrorConsoleAction, true);
	addSeparator();

	Menu *addNewMenu(new Menu(NoMenuRole, this));
	Action *addNewAction(addAction());
	addNewAction->setMenu(addNewMenu);
	addNewAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add New"));

	Action *addToolBarAction(addNewMenu->addAction());
	addToolBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Toolbar…"));

	Action *addBookmarksBarAction(addNewMenu->addAction());
	addBookmarksBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Bookmarks Bar…"));

	Action *addSideBarAction(addNewMenu->addAction());
	addSideBarAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Add Sidebar…"));

	addSeparator();
	addAction(ActionsManager::LockToolBarsAction, true);
	addSeparator();
	addAction(ActionsManager::ResetToolBarsAction, true);

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
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	const QString userAgent(mainWindow ? mainWindow->getWindowsManager()->getOption(SettingsManager::Network_UserAgentOption).toString() : QString());
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
			Action *action(addAction());
			action->setData(userAgents.at(i));
			action->setText(Utils::elideText(definition.getTitle(), this));

			if (definition.isFolder)
			{
				action->setIcon(ThemesManager::getIcon(QLatin1String("inode-directory")));

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

		Action *customAction(addAction());
		customAction->setData(QLatin1String("custom"));
		customAction->setCheckable(true);
		customAction->setChecked(userAgent.startsWith(QLatin1String("custom;")));
		customAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Custom User Agent…"));

		m_actionGroup->addAction(customAction);
	}
}

void Menu::populateWindowsMenu()
{
	if (actions().isEmpty())
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			connect(mainWindow->getWindowsManager(), SIGNAL(windowAdded(quint64)), this, SLOT(populateWindowsMenu()));
			connect(mainWindow->getWindowsManager(), SIGNAL(windowRemoved(quint64)), this, SLOT(populateWindowsMenu()));
			connect(mainWindow->getWindowsManager(), SIGNAL(windowTitleChanged(QString)), this, SLOT(populateWindowsMenu()));
		}

		disconnect(this, SIGNAL(aboutToShow()), this, SLOT(populateWindowsMenu()));
	}

	clear();

	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (!mainWindow)
	{
		return;
	}

	for (int i = 0; i < mainWindow->getWindowsManager()->getWindowCount(); ++i)
	{
		Window *window(mainWindow->getWindowsManager()->getWindowByIndex(i));

		if (window)
		{
			Action *action(addAction());
			action->setData(window->getIdentifier());
			action->setIcon(window->getIcon());

			if (window->getTitle().isEmpty())
			{
				action->setOverrideText(QT_TRANSLATE_NOOP("actions", "(Untitled)"));
			}
			else
			{
				action->setText(Utils::elideText(window->getTitle()));
			}
		}
	}
}

void Menu::clearModelMenu()
{
	const int offset((m_role == BookmarksMenuRole && menuAction() && !menuAction()->data().toModelIndex().isValid()) ? 3 : 0);

	for (int i = (actions().count() - 1); i >= offset; --i)
	{
		actions().at(i)->deleteLater();

		removeAction(actions().at(i));
	}

	if (m_role != BookmarksMenuRole && menuAction())
	{
		const QAbstractItemModel *model(menuAction()->data().toModelIndex().model());

		if (model)
		{
			menuAction()->setEnabled(model->rowCount(menuAction()->data().toModelIndex()) > 0);
		}
	}

	connect(this, SIGNAL(aboutToShow()), this, SLOT(populateModelMenu()));
}

void Menu::clearClosedWindows()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		mainWindow->getWindowsManager()->clearClosedWindows();
	}

	SessionsManager::clearClosedWindows();
}

void Menu::restoreClosedWindow()
{
	QAction *action(qobject_cast<QAction*>(sender()));
	const int index((action && action->data().type() == QVariant::Int) ? action->data().toInt() : 0);

	if (index > 0)
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

		if (mainWindow)
		{
			mainWindow->getWindowsManager()->restore(index - 1);
		}
	}
	else if (index < 0)
	{
		SessionsManager::restoreClosedWindow(-index - 1);
	}
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

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action && action->data().type() == QVariant::ModelIndex)
	{
		m_bookmark = BookmarksManager::getModel()->getBookmark(action->data().toModelIndex());
	}

	if (mainWindow)
	{
		const WindowsManager::OpenHints hints(action ? static_cast<WindowsManager::OpenHints>(action->data().toInt()) : WindowsManager::DefaultOpen);

		mainWindow->getWindowsManager()->open(m_bookmark, ((hints == WindowsManager::DefaultOpen) ? WindowsManager::calculateOpenHints() : hints));
	}

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

void Menu::selectOption(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (mainWindow)
	{
		mainWindow->getWindowsManager()->setOption(m_option, action->data().toString());
	}
}

void Menu::selectStyleSheet(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (!mainWindow)
	{
		return;
	}

	Window *window(mainWindow->getWindowsManager()->getWindowByIndex(-1));

	if (window && action)
	{
		window->getContentsWidget()->setActiveStyleSheet(action->data().isNull() ? action->text() : QString());
	}
}

void Menu::selectWindow(QAction *action)
{
	if (action)
	{
		QVariantMap parameters;
		parameters[QLatin1String("window")] = action->data().toULongLong();

		ActionsManager::triggerAction(ActionsManager::ActivateTabAction, this, parameters);
	}
}

void Menu::updateClosedWindowsMenu()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	setEnabled((mainWindow && mainWindow->getWindowsManager()->getClosedWindows().count() > 0) || SessionsManager::getClosedWindows().count() > 0);
}

void Menu::setToolBarVisibility(bool visible)
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action)
	{
		ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(action->data().toInt()));
		definition.normalVisibility = (visible ? ToolBarsManager::AlwaysVisibleToolBar : ToolBarsManager::AlwaysHiddenToolBar);

		ToolBarsManager::setToolBar(definition);
	}
}

void Menu::setTitle(const QString &title)
{
	m_title = title;

	QMenu::setTitle(QCoreApplication::translate("actions", m_title.toUtf8().constData()));
}

Action* Menu::addAction(int identifier, bool useGlobal)
{
	Action *action(useGlobal ? ActionsManager::getAction(identifier, this) : new Action(identifier, this));

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

	if (identifier == QLatin1String("ProxyMenu"))
	{
		return ProxyMenuRole;
	}

	if (identifier == QLatin1String("SearchMenu"))
	{
		return SearchMenuRole;
	}

	if (identifier == QLatin1String("SessionsMenu"))
	{
		return SessionsMenuRole;
	}

	if (identifier == QLatin1String("StyleSheetsMenu"))
	{
		return StyleSheetsMenuRole;
	}

	if (identifier == QLatin1String("ToolBarsMenu"))
	{
		return ToolBarsMenuRole;
	}

	if (identifier == QLatin1String("UserAgentMenu"))
	{
		return UserAgentMenuRole;
	}

	if (identifier == QLatin1String("ValidateMenu"))
	{
		return ValidateMenuRole;
	}

	if (identifier == QLatin1String("WindowsMenu"))
	{
		return WindowsMenuRole;
	}

	return NoMenuRole;
}

}
