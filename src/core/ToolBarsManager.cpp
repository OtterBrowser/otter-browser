/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ToolBarsManager.h"
#include "SessionsManager.h"
#include "Utils.h"
#include "../ui/BookmarksComboBoxWidget.h"
#include "../ui/BookmarksBarDialog.h"
#include "../ui/ToolBarDialog.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QVector>
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ToolBarsManager* ToolBarsManager::m_instance = NULL;
QMap<int, QString> ToolBarsManager::m_identifiers;
QVector<ToolBarDefinition> ToolBarsManager::m_definitions;
bool ToolBarsManager::m_areToolBarsLocked = false;

ToolBarsManager::ToolBarsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Menu Bar"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Tab Bar"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Navigation Bar"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Status Bar"));
}

void ToolBarsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ToolBarsManager(parent);
		m_areToolBarsLocked = SettingsManager::getValue(QLatin1String("Interface/LockToolBars")).toBool();
	}
}

void ToolBarsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (!m_definitions.isEmpty())
		{
			QJsonArray definitions;

			for (int i = 0; i < m_definitions.count(); ++i)
			{
				if ((m_definitions[i].isDefault && !m_definitions[i].canReset) || m_definitions[i].wasRemoved)
				{
					continue;
				}

				QString identifier;

				if (m_identifiers.contains(m_definitions[i].identifier))
				{
					identifier = m_identifiers[m_definitions[i].identifier];
				}
				else
				{
					switch (m_definitions[i].identifier)
					{
						case MenuBar:
							identifier = QLatin1String("MenuBar");

							break;
						case TabBar:
							identifier = QLatin1String("TabBar");

							break;
						case NavigationBar:
							identifier = QLatin1String("NavigationBar");

							break;
						case StatusBar:
							identifier = QLatin1String("StatusBar");

							break;
					}
				}

				if (identifier.isEmpty())
				{
					continue;
				}

				QJsonObject definition;
				definition.insert(QLatin1String("identifier"), QJsonValue(identifier));
				definition.insert(QLatin1String("title"), QJsonValue(m_definitions[i].title));

				if (!m_definitions[i].bookmarksPath.isEmpty())
				{
					definition.insert(QLatin1String("bookmarksPath"), QJsonValue(m_definitions[i].bookmarksPath));
				}

				QString visibility;

				switch (m_definitions[i].visibility)
				{
					case AlwaysHiddenToolBar:
						visibility = QLatin1String("hidden");

						break;
					case AutoVisibilityToolBar:
						visibility = QLatin1String("auto");

						break;
					default:
						visibility = QLatin1String("visible");

						break;
				}

				definition.insert(QLatin1String("visibility"), QJsonValue(visibility));

				if (m_definitions[i].location != Qt::NoToolBarArea)
				{
					QString location;

					switch (m_definitions[i].location)
					{
						case Qt::BottomToolBarArea:
							location = QLatin1String("bottom");

							break;
						case Qt::LeftToolBarArea:
							location = QLatin1String("left");

							break;
						case Qt::RightToolBarArea:
							location = QLatin1String("right");

							break;
						default:
							location = QLatin1String("top");

							break;
					}

					definition.insert(QLatin1String("location"), QJsonValue(location));
				}

				QString buttonStyle;

				switch (m_definitions[i].buttonStyle)
				{
					case Qt::ToolButtonFollowStyle:
						buttonStyle = QLatin1String("auto");

						break;
					case Qt::ToolButtonTextOnly:
						buttonStyle = QLatin1String("textOnly");

						break;
					case Qt::ToolButtonTextBesideIcon:
						buttonStyle = QLatin1String("textBesideIcon");

						break;
					case Qt::ToolButtonTextUnderIcon:
						buttonStyle = QLatin1String("textUnderIcon");

						break;
					default:
						buttonStyle = QLatin1String("iconOnly");

						break;
				}

				definition.insert(QLatin1String("buttonStyle"), QJsonValue(buttonStyle));

				if (m_definitions[i].iconSize > 0)
				{
					definition.insert(QLatin1String("iconSize"), QJsonValue(m_definitions[i].iconSize));
				}

				if (m_definitions[i].maximumButtonSize > 0)
				{
					definition.insert(QLatin1String("iconSize"), QJsonValue(m_definitions[i].maximumButtonSize));
				}

				if (!m_definitions[i].actions.isEmpty())
				{
					QJsonArray actions;

					for (int j = 0; j < m_definitions[i].actions.count(); ++j)
					{
						if (m_definitions[i].actions.at(j).options.isEmpty())
						{
							actions.append(QJsonValue(m_definitions[i].actions.at(j).action));
						}
						else
						{
							QJsonObject action;
							action.insert(QLatin1String("identifier"), QJsonValue(m_definitions[i].actions.at(j).action));

							QJsonObject options;
							QVariantMap::const_iterator optionsIterator;

							for (optionsIterator = m_definitions[i].actions.at(j).options.begin(); optionsIterator != m_definitions[i].actions.at(j).options.end(); ++optionsIterator)
							{
								options.insert(optionsIterator.key(), QJsonValue::fromVariant(optionsIterator.value()));
							}

							action.insert(QLatin1String("options"), options);

							actions.append(action);
						}
					}

					definition.insert(QLatin1String("actions"), actions);
				}

				definitions.append(definition);
			}

			QJsonDocument document;
			document.setArray(definitions);

			QFile file(SessionsManager::getWritableDataPath(QLatin1String("toolBars.json")));

			if (file.open(QFile::WriteOnly))
			{
				file.write(document.toJson(QJsonDocument::Indented));
				file.close();
			}
		}
	}
}

void ToolBarsManager::optionChanged(const QString &key, const QVariant &value)
{
	if (key == QLatin1String("Interface/LockToolBars"))
	{
		m_areToolBarsLocked = value.toBool();

		emit toolBarsLockedChanged(m_areToolBarsLocked);
	}
}

void ToolBarsManager::addToolBar()
{
	ToolBarDialog dialog;

	if (dialog.exec() == QDialog::Accepted)
	{
		setToolBar(dialog.getDefinition());
	}
}

void ToolBarsManager::addBookmarksBar()
{
	BookmarksBarDialog dialog;

	if (dialog.exec() == QDialog::Accepted)
	{
		setToolBar(dialog.getDefinition());
	}
}

void ToolBarsManager::configureToolBar(int identifier)
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action && identifier < 0)
	{
		identifier = action->data().toInt();
	}

	if (identifier >= 0 && identifier < (m_definitions.count() - 1))
	{
		ToolBarDialog dialog(identifier);

		if (dialog.exec() == QDialog::Accepted)
		{
			setToolBar(dialog.getDefinition());
		}
	}
}

void ToolBarsManager::resetToolBar(int identifier)
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action && identifier < 0)
	{
		identifier = action->data().toInt();
	}

//TODO
}

void ToolBarsManager::removeToolBar(int identifier)
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action && identifier < 0)
	{
		identifier = action->data().toInt();
	}

	if (identifier >= 0 && identifier < (m_definitions.count() - 1) && identifier >= OtherToolBar && QMessageBox::question(NULL, tr("Remove Toolbar"), tr("Do you really want to remove this toolbar?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Yes)
	{
		m_definitions[identifier].wasRemoved = true;
		m_definitions[identifier].title = QString();
		m_definitions[identifier].actions.clear();

		m_instance->scheduleSave();

		emit m_instance->toolBarRemoved(identifier);
	}
}

void ToolBarsManager::resetToolBars()
{
//TODO
}

void ToolBarsManager::setToolBar(ToolBarDefinition definition)
{
	int identifier = definition.identifier;

	if (identifier < 0 || identifier >= m_definitions.count())
	{
		QStringList toolBars = m_identifiers.values();
		toolBars << QLatin1String("MenuBar") << QLatin1String("TabBar") << QLatin1String("NavigationBar") << QLatin1String("StatusBar");

		identifier = m_definitions.count();

		m_identifiers[identifier] = Utils::createIdentifier(QLatin1String("CustomBar"), toolBars, false);

		m_definitions.append(definition);

		m_definitions[identifier].identifier = identifier;
		m_definitions[identifier].location = Qt::TopToolBarArea;

		emit m_instance->toolBarAdded(identifier);
	}
	else
	{
		m_definitions[identifier] = definition;

		emit m_instance->toolBarModified(identifier);
	}

	m_instance->scheduleSave();
}

void ToolBarsManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void ToolBarsManager::setToolBarsLocked(bool locked)
{
	SettingsManager::setValue(QLatin1String("Interface/LockToolBars"), locked);

	emit m_instance->toolBarsLockedChanged(locked);
}

ToolBarsManager* ToolBarsManager::getInstance()
{
	return m_instance;
}

ToolBarDefinition ToolBarsManager::getToolBarDefinition(int identifier)
{
	if (m_definitions.isEmpty())
	{
		getToolBarDefinitions();
	}

	if (identifier >= 0 && identifier < m_definitions.count())
	{
		return m_definitions[identifier];
	}

	return ToolBarDefinition();
}

QHash<QString, ToolBarDefinition> ToolBarsManager::loadToolBars(const QString &path, bool isDefault)
{
	QHash<QString, ToolBarDefinition> definitions;
	QFile file(path);

	if (!file.open(QFile::ReadOnly))
	{
		return definitions;
	}

	const QJsonArray toolBars = QJsonDocument::fromJson(file.readAll()).array();

	for (int i = 0; i < toolBars.count(); ++i)
	{
		const QJsonObject toolBarObject = toolBars.at(i).toObject();
		const QJsonArray actions = toolBarObject.value(QLatin1String("actions")).toArray();
		const QString identifier = toolBarObject.value(QLatin1String("identifier")).toString();
		const QString visibility = toolBarObject.value(QLatin1String("visibility")).toString();
		const QString location = toolBarObject.value(QLatin1String("location")).toString();
		const QString buttonStyle = toolBarObject.value(QLatin1String("buttonStyle")).toString();
		ToolBarDefinition toolBar;
		toolBar.title = toolBarObject.value(QLatin1String("title")).toString();
		toolBar.bookmarksPath = toolBarObject.value(QLatin1String("bookmarksPath")).toString();
		toolBar.iconSize = toolBarObject.value(QLatin1String("iconSize")).toInt();
		toolBar.maximumButtonSize = toolBarObject.value(QLatin1String("maximumButtonSize")).toInt();
		toolBar.isDefault = isDefault;

		if (isDefault)
		{
			toolBar.title = QCoreApplication::translate("actions", toolBar.title.toUtf8());
		}

		if (visibility == QLatin1String("hidden"))
		{
			toolBar.visibility = AlwaysHiddenToolBar;
		}
		else if (visibility == QLatin1String("auto"))
		{
			toolBar.visibility = AutoVisibilityToolBar;
		}

		if (location == QLatin1String("top"))
		{
			toolBar.location = Qt::TopToolBarArea;
		}
		else if (location == QLatin1String("bottom"))
		{
			toolBar.location = Qt::BottomToolBarArea;
		}
		else if (location == QLatin1String("left"))
		{
			toolBar.location = Qt::LeftToolBarArea;
		}
		else if (location == QLatin1String("right"))
		{
			toolBar.location = Qt::RightToolBarArea;
		}

		if (buttonStyle == QLatin1String("auto"))
		{
			toolBar.buttonStyle = Qt::ToolButtonFollowStyle;
		}
		else if (buttonStyle == QLatin1String("textOnly"))
		{
			toolBar.buttonStyle = Qt::ToolButtonTextOnly;
		}
		else if (buttonStyle == QLatin1String("textBesideIcon"))
		{
			toolBar.buttonStyle = Qt::ToolButtonTextBesideIcon;
		}
		else if (buttonStyle == QLatin1String("textUnderIcon"))
		{
			toolBar.buttonStyle = Qt::ToolButtonTextUnderIcon;
		}

		for (int j = 0; j < actions.count(); ++j)
		{
			ToolBarActionDefinition action;

			if (actions.at(j).isObject())
			{
				const QJsonObject actionObject = actions.at(j).toObject();

				action.action = actionObject.value(QLatin1String("identifier")).toString();
				action.options = actionObject.value(QLatin1String("options")).toObject().toVariantMap();
			}
			else
			{
				action.action = actions.at(j).toString();
			}

			toolBar.actions.append(action);
		}

		definitions[identifier] = toolBar;
	}

	return definitions;
}

QVector<ToolBarDefinition> ToolBarsManager::getToolBarDefinitions()
{
	if (m_definitions.isEmpty())
	{
		const QHash<QString, ToolBarDefinition> defaultDefinitions = loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true);

		m_definitions.reserve(4);
		m_definitions.append(defaultDefinitions[QLatin1String("MenuBar")]);
		m_definitions.append(defaultDefinitions[QLatin1String("TabBar")]);
		m_definitions.append(defaultDefinitions[QLatin1String("NavigationBar")]);
		m_definitions.append(defaultDefinitions[QLatin1String("StatusBar")]);

		const QString customToolBarsPath = SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"));

		if (QFile::exists(customToolBarsPath))
		{
			const QHash<QString, ToolBarDefinition> customDefinitions = loadToolBars(customToolBarsPath, false);
			QHash<QString, ToolBarDefinition>::const_iterator iterator;

			for (iterator = customDefinitions.constBegin(); iterator != customDefinitions.constEnd(); ++iterator)
			{
				int identifier = -1;

				if (iterator.key() == QLatin1String("MenuBar"))
				{
					identifier = MenuBar;
				}
				else if (iterator.key() == QLatin1String("TabBar"))
				{
					identifier = TabBar;
				}
				else if (iterator.key() == QLatin1String("NavigationBar"))
				{
					identifier = NavigationBar;
				}
				else if (iterator.key() == QLatin1String("StatusBar"))
				{
					identifier = StatusBar;
				}

				if (identifier >= 0)
				{
					m_definitions[identifier] = iterator.value();
					m_definitions[identifier].canReset = true;
				}
				else
				{
					identifier = m_definitions.count();

					m_identifiers[identifier] = iterator.key();
\
					m_definitions.append(iterator.value());
					m_definitions[identifier].identifier = identifier;
				}

				m_definitions[identifier].identifier = identifier;
			}
		}

		bool hasMenuBar = false;

		for (int i = 0; i < m_definitions[MenuBar].actions.count(); ++i)
		{
			if (m_definitions[MenuBar].actions.at(i).action == QLatin1String("MenuBarWidget"))
			{
				hasMenuBar = true;

				break;
			}
		}

		if (!hasMenuBar)
		{
			ToolBarActionDefinition definition;
			definition.action = QLatin1String("MenuBarWidget");

			m_definitions[MenuBar].actions.prepend(definition);
		}

		bool hasTabBar = false;

		for (int i = 0; i < m_definitions[TabBar].actions.count(); ++i)
		{
			if (m_definitions[TabBar].actions.at(i).action == QLatin1String("TabBarWidget"))
			{
				hasTabBar = true;

				break;
			}
		}

		if (!hasTabBar)
		{
			ToolBarActionDefinition definition;
			definition.action = QLatin1String("TabBarWidget");

			m_definitions[TabBar].actions.prepend(definition);
		}
	}

	QVector<ToolBarDefinition> definitions;
	definitions.reserve(m_definitions.count());

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		if (!m_definitions.at(i).wasRemoved)
		{
			definitions.append(m_definitions.at(i));
		}
	}

	return definitions;
}

bool ToolBarsManager::areToolBarsLocked()
{
	return m_areToolBarsLocked;
}

}
