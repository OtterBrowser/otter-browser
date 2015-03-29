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

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QVector>

namespace Otter
{

ToolBarsManager* ToolBarsManager::m_instance = NULL;
QMap<int, QString> ToolBarsManager::m_identifiers;
QVector<ToolBarDefinition> ToolBarsManager::m_definitions;

ToolBarsManager::ToolBarsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void ToolBarsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ToolBarsManager(parent);
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
				if (m_definitions[i].isDefault)
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

void ToolBarsManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
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
		const QString location = toolBarObject.value(QLatin1String("location")).toString();
		const QString buttonStyle = toolBarObject.value(QLatin1String("buttonStyle")).toString();
		ToolBarDefinition toolBar;
		toolBar.title = toolBarObject.value(QLatin1String("title")).toString();
		toolBar.bookmarksPath = toolBarObject.value(QLatin1String("bookmarksPath")).toString();
		toolBar.iconSize = toolBarObject.value(QLatin1String("iconSize")).toInt();
		toolBar.maximumButtonSize = toolBarObject.value(QLatin1String("maximumButtonSize")).toInt();
		toolBar.isDefault = isDefault;

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
				}
				else
				{
					identifier = m_definitions.count();

					m_identifiers[identifier] = iterator.key();

					m_definitions.append(iterator.value());
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

	return m_definitions;
}

}
