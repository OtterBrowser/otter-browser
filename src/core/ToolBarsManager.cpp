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

namespace Otter
{

ToolBarsManager* ToolBarsManager::m_instance = NULL;
QHash<QString, ToolBarDefinition> ToolBarsManager::m_definitions;

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
			QHash<QString, ToolBarDefinition>::iterator definitionsIterator;

			for (definitionsIterator = m_definitions.begin(); definitionsIterator != m_definitions.end(); ++definitionsIterator)
			{
				if (definitionsIterator.value().isDefault)
				{
					continue;
				}

				QJsonObject definition;
				definition.insert(QLatin1String("identifier"), QJsonValue(definitionsIterator.value().identifier));
				definition.insert(QLatin1String("title"), QJsonValue(definitionsIterator.value().title));

				if (!definitionsIterator.value().bookmarksPath.isEmpty())
				{
					definition.insert(QLatin1String("bookmarksPath"), QJsonValue(definitionsIterator.value().bookmarksPath));
				}

				if (definitionsIterator.value().location != Qt::NoToolBarArea)
				{
					QString location;

					switch (definitionsIterator.value().location)
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

				switch (definitionsIterator.value().buttonStyle)
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

				if (definitionsIterator.value().iconSize > 0)
				{
					definition.insert(QLatin1String("iconSize"), QJsonValue(definitionsIterator.value().iconSize));
				}

				if (definitionsIterator.value().maximumButtonSize > 0)
				{
					definition.insert(QLatin1String("iconSize"), QJsonValue(definitionsIterator.value().maximumButtonSize));
				}

				if (!definitionsIterator.value().actions.isEmpty())
				{
					QJsonArray actions;

					for (int i = 0; i < definitionsIterator.value().actions.count(); ++i)
					{
						if (definitionsIterator.value().actions.at(i).options.isEmpty())
						{
							actions.append(QJsonValue(definitionsIterator.value().actions.at(i).action));
						}
						else
						{
							QJsonObject action;
							action.insert(QLatin1String("identifier"), QJsonValue(definitionsIterator.value().actions.at(i).action));

							QJsonObject options;
							QVariantMap::const_iterator optionsIterator;

							for (optionsIterator = definitionsIterator.value().actions.at(i).options.begin(); optionsIterator != definitionsIterator.value().actions.at(i).options.end(); ++optionsIterator)
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

ToolBarDefinition ToolBarsManager::getToolBarDefinition(const QString &identifier)
{
	if (m_definitions.isEmpty())
	{
		getToolBarDefinitions();
	}

	if (!m_definitions.contains(identifier))
	{
		ToolBarDefinition definition;
		definition.identifier = identifier;

		return definition;
	}

	return m_definitions[identifier];
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
		const QString location = toolBarObject.value(QLatin1String("location")).toString();
		const QString buttonStyle = toolBarObject.value(QLatin1String("buttonStyle")).toString();
		ToolBarDefinition toolBar;
		toolBar.identifier = toolBarObject.value(QLatin1String("identifier")).toString();
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

		definitions[toolBar.identifier] = toolBar;
	}

	return definitions;
}

QList<ToolBarDefinition> ToolBarsManager::getToolBarDefinitions()
{
	if (m_definitions.isEmpty())
	{
		m_definitions = loadToolBars(QLatin1String(":/other/toolBars.json"), true);

		const QString customToolBarsPath = SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"));

		if (QFile::exists(customToolBarsPath))
		{
			const QHash<QString, ToolBarDefinition> customToolBarDefinitions = loadToolBars(customToolBarsPath, false);
			QHash<QString, ToolBarDefinition>::const_iterator iterator;

			for (iterator = customToolBarDefinitions.constBegin(); iterator != customToolBarDefinitions.constEnd(); ++iterator)
			{
				m_definitions[iterator.key()] = iterator.value();
			}
		}

		if (m_definitions.contains(QLatin1String("MenuBar")))
		{
			bool hasMenuBar = false;

			for (int i = 0; i < m_definitions[QLatin1String("MenuBar")].actions.count(); ++i)
			{
				if (m_definitions[QLatin1String("MenuBar")].actions.at(i).action == QLatin1String("MenuBarWidget"))
				{
					hasMenuBar = true;

					break;
				}
			}

			if (!hasMenuBar)
			{
				ToolBarActionDefinition definition;
				definition.action = QLatin1String("MenuBar");

				m_definitions[QLatin1String("MenuBar")].actions.prepend(definition);
			}
		}

		if (m_definitions.contains(QLatin1String("TabBar")))
		{
			bool hasTabBar = false;

			for (int i = 0; i < m_definitions[QLatin1String("TabBar")].actions.count(); ++i)
			{
				if (m_definitions[QLatin1String("TabBar")].actions.at(i).action == QLatin1String("TabBarWidget"))
				{
					hasTabBar = true;

					break;
				}
			}

			if (!hasTabBar)
			{
				ToolBarActionDefinition definition;
				definition.action = QLatin1String("TabBar");

				m_definitions[QLatin1String("TabBar")].actions.prepend(definition);
			}
		}
	}

	return m_definitions.values();
}

}
