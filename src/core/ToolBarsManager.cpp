/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ToolBarsManager.h"
#include "JsonSettings.h"
#include "SessionsManager.h"
#include "Utils.h"
#include "../ui/ToolBarDialog.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ToolBarsManager* ToolBarsManager::m_instance(nullptr);
QMap<int, QString> ToolBarsManager::m_identifiers;
QVector<ToolBarsManager::ToolBarDefinition> ToolBarsManager::m_definitions;
int ToolBarsManager::m_toolBarIdentifierEnumerator(0);
bool ToolBarsManager::m_areToolBarsLocked(false);
bool ToolBarsManager::m_isLoading(false);

ToolBarsManager::ToolBarsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Menu Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Bookmarks Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Tab Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Address Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Navigation Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Progress Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Sidebar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Status Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Error Console"))

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, [&](int identifier, const QVariant &value)
	{
		if (identifier == SettingsManager::Interface_LockToolBarsOption)
		{
			m_areToolBarsLocked = value.toBool();

			emit toolBarsLockedChanged(m_areToolBarsLocked);
		}
	});
}

void ToolBarsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ToolBarsManager(QCoreApplication::instance());
		m_toolBarIdentifierEnumerator = staticMetaObject.indexOfEnumerator(QLatin1String("ToolBarIdentifier").data());
		m_areToolBarsLocked = SettingsManager::getOption(SettingsManager::Interface_LockToolBarsOption).toBool();
	}
}

void ToolBarsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (m_definitions.isEmpty())
		{
			return;
		}

		QJsonArray definitionsArray;
		const QMap<ToolBarVisibility, QString> visibilityModes({{AlwaysVisibleToolBar, QLatin1String("visible")}, {OnHoverVisibleToolBar, QLatin1String("hover")}, {AutoVisibilityToolBar, QLatin1String("auto")}, {AlwaysHiddenToolBar, QLatin1String("hidden")}});

		for (int i = 0; i < m_definitions.count(); ++i)
		{
			const ToolBarDefinition definition(m_definitions.at(i));

			if (definition.isDefault || definition.wasRemoved)
			{
				continue;
			}

			const QString identifier(getToolBarName(definition.identifier));

			if (identifier.isEmpty())
			{
				continue;
			}

			QJsonObject definitionObject({{QLatin1String("identifier"), QJsonValue(identifier)}, {QLatin1String("title"), QJsonValue(definition.title)}, {QLatin1String("normalVisibility"), QJsonValue(visibilityModes.value(definition.normalVisibility))}, {QLatin1String("fullScreenVisibility"), QJsonValue(visibilityModes.value(definition.fullScreenVisibility))}});
			QString location;
			QString buttonStyle;

			switch (definition.type)
			{
				case BookmarksBarType:
					definitionObject.insert(QLatin1String("bookmarksPath"), QJsonValue(definition.bookmarksPath));

					break;
				case SideBarType:
					definitionObject.insert(QLatin1String("currentPanel"), QJsonValue(definition.currentPanel));
					definitionObject.insert(QLatin1String("panels"), QJsonArray::fromStringList(definition.panels));

					break;
				default:
					break;
			}

			switch (definition.location)
			{
				case Qt::LeftToolBarArea:
					location = QLatin1String("left");

					break;
				case Qt::RightToolBarArea:
					location = QLatin1String("right");

					break;
				case Qt::TopToolBarArea:
					location = QLatin1String("top");

					break;
				case Qt::BottomToolBarArea:
					location = QLatin1String("bottom");

					break;
				default:
					break;
			}

			if (!location.isEmpty())
			{
				definitionObject.insert(QLatin1String("location"), location);
			}

			switch (definition.buttonStyle)
			{
				case Qt::ToolButtonTextOnly:
					buttonStyle = QLatin1String("textOnly");

					break;
				case Qt::ToolButtonTextBesideIcon:
					buttonStyle = QLatin1String("textBesideIcon");

					break;
				case Qt::ToolButtonTextUnderIcon:
					buttonStyle = QLatin1String("textUnderIcon");

					break;
				case Qt::ToolButtonFollowStyle:
					buttonStyle = QLatin1String("auto");

					break;
				default:
					buttonStyle = QLatin1String("iconOnly");

					break;
			}

			definitionObject.insert(QLatin1String("buttonStyle"), buttonStyle);

			if (definition.iconSize > 0)
			{
				definitionObject.insert(QLatin1String("iconSize"), QJsonValue(definition.iconSize));
			}

			if (definition.maximumButtonSize > 0)
			{
				definitionObject.insert(QLatin1String("maximumButtonSize"), QJsonValue(definition.maximumButtonSize));
			}

			if (definition.panelSize > 0)
			{
				definitionObject.insert(QLatin1String("panelSize"), QJsonValue(definition.panelSize));
			}

			definitionObject.insert(QLatin1String("row"), QJsonValue(definition.row));

			if (definition.hasToggle)
			{
				definitionObject.insert(QLatin1String("hasToggle"), QJsonValue(true));
			}

			if (!definition.entries.isEmpty())
			{
				QJsonArray actionsArray;

				for (int j = 0; j < definition.entries.count(); ++j)
				{
					actionsArray.append(encodeEntry(definition.entries.at(j)));
				}

				definitionObject.insert(QLatin1String("actions"), actionsArray);
			}

			definitionsArray.append(definitionObject);
		}

		JsonSettings settings;
		settings.setArray(definitionsArray);
		settings.save(SessionsManager::getWritableDataPath(QLatin1String("toolBars.json")));
	}
}

void ToolBarsManager::ensureInitialized()
{
	if (m_isLoading || !m_definitions.isEmpty())
	{
		return;
	}

	m_isLoading = true;

	const QString bundledToolBarsPath(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true));
	const QHash<QString, ToolBarsManager::ToolBarDefinition> bundledDefinitions(loadToolBars(bundledToolBarsPath, true));

	m_definitions.reserve(OtherToolBar);

	for (int i = 0; i < OtherToolBar; ++i)
	{
		m_definitions.append(bundledDefinitions.value(getToolBarName(i)));
	}

	const QString localToolBarsPath(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json")));

	if (localToolBarsPath != bundledToolBarsPath && QFile::exists(localToolBarsPath))
	{
		const QHash<QString, ToolBarDefinition> localDefinitions(loadToolBars(localToolBarsPath, false));

		if (!localDefinitions.isEmpty())
		{
			QHash<QString, ToolBarDefinition>::const_iterator iterator;

			for (iterator = localDefinitions.constBegin(); iterator != localDefinitions.constEnd(); ++iterator)
			{
				int identifier(getToolBarIdentifier(iterator.key()));

				if (identifier >= 0)
				{
					ToolBarDefinition definition(iterator.value());
					definition.identifier = identifier;

					m_definitions[identifier] = definition;
				}
				else
				{
					identifier = m_definitions.count();

					m_identifiers[identifier] = iterator.key();

					m_definitions.append(iterator.value());
					m_definitions[identifier].identifier = identifier;
				}
			}
		}
	}

	for (int i = 0; i < OtherToolBar; ++i)
	{
		if (m_definitions.count() > i)
		{
			m_definitions[i].identifier = i;
		}
	}

	const QVector<ToolBarDefinition::Entry> menuBarEntries(m_definitions[MenuBar].entries);
	bool hasMenuBar(false);

	for (int i = 0; i < menuBarEntries.count(); ++i)
	{
		if (menuBarEntries.at(i).action == QLatin1String("MenuBarWidget"))
		{
			hasMenuBar = true;

			break;
		}
	}

	if (!hasMenuBar)
	{
		ToolBarDefinition::Entry definition;
		definition.action = QLatin1String("MenuBarWidget");

		m_definitions[MenuBar].entries.prepend(definition);
	}

	const QVector<ToolBarDefinition::Entry> tabBarEntries(m_definitions[TabBar].entries);
	bool hasTabBar(false);

	for (int i = 0; i < tabBarEntries.count(); ++i)
	{
		if (tabBarEntries.at(i).action == QLatin1String("TabBarWidget"))
		{
			hasTabBar = true;

			break;
		}
	}

	if (!hasTabBar)
	{
		ToolBarDefinition::Entry definition;
		definition.action = QLatin1String("TabBarWidget");

		m_definitions[TabBar].entries.prepend(definition);
	}

	m_isLoading = false;
}

void ToolBarsManager::addToolBar(ToolBarsManager::ToolBarType type)
{
	ToolBarDefinition definition;
	definition.type = type;

	switch (type)
	{
		case BookmarksBarType:
			definition.bookmarksPath = QLatin1Char('/');

			break;
		case SideBarType:
			definition.panels = QStringList({QLatin1String("bookmarks"), QLatin1String("history"), QLatin1String("feeds"), QLatin1String("notes"), QLatin1String("passwords"), QLatin1String("transfers")});
			definition.location = Qt::LeftToolBarArea;

			break;
		default:
			break;
	}

	ToolBarDialog dialog(definition);

	if (dialog.exec() == QDialog::Accepted)
	{
		setToolBar(dialog.getDefinition());
	}
}

void ToolBarsManager::configureToolBar(int identifier)
{
	if (identifier >= 0 && identifier < m_definitions.count())
	{
		ToolBarDialog dialog(getToolBarDefinition(identifier));

		if (dialog.exec() == QDialog::Accepted)
		{
			setToolBar(dialog.getDefinition());
		}
	}
}

void ToolBarsManager::resetToolBar(int identifier)
{
	if (identifier >= 0 && identifier < OtherToolBar && QMessageBox::question(nullptr, tr("Reset Toolbar"), tr("Do you really want to reset this toolbar to default configuration?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		setToolBar(loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true).value(getToolBarName(identifier)));
	}
}

void ToolBarsManager::removeToolBar(int identifier)
{
	if (identifier >= 0 && identifier < m_definitions.count() && identifier >= OtherToolBar && QMessageBox::question(nullptr, tr("Remove Toolbar"), tr("Do you really want to remove this toolbar?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		m_definitions[identifier].markAsRemoved();

		m_instance->scheduleSave();

		emit m_instance->toolBarRemoved(identifier);
	}
}

void ToolBarsManager::resetToolBars()
{
	if (QMessageBox::question(nullptr, tr("Reset Toolbars"), tr("Do you really want to reset all toolbars to default configuration?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		return;
	}

	ensureInitialized();

	const QList<int> customToolBars(m_identifiers.keys());

	for (int i = 0; i < customToolBars.count(); ++i)
	{
		emit m_instance->toolBarRemoved(customToolBars.at(i));
	}

	const QHash<QString, ToolBarDefinition> definitions(loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true));

	m_identifiers.clear();
	m_definitions.clear();
	m_definitions.reserve(definitions.count());

	for (int i = 0; i < OtherToolBar; ++i)
	{
		m_definitions.append(definitions.value(getToolBarName(i)));

		emit m_instance->toolBarModified(i);
	}

	emit m_instance->toolBarMoved(TabBar);

	m_instance->scheduleSave();
}

void ToolBarsManager::setToolBar(ToolBarsManager::ToolBarDefinition definition)
{
	ensureInitialized();

	int identifier(definition.identifier);

	if (definition.isValid() && identifier < OtherToolBar)
	{
		const ToolBarsManager::ToolBarDefinition defaultDefinition(loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true).value(getToolBarName(identifier)));

		definition.isDefault = (defaultDefinition.isValid() && definition == defaultDefinition);
	}

	if (identifier < 0 || identifier >= m_definitions.count())
	{
		QStringList toolBars;
		toolBars.reserve(m_definitions.count());

		for (int i = 0; i < OtherToolBar; ++i)
		{
			toolBars.append(getToolBarName(i));
		}

		toolBars.append(m_identifiers.values());

		identifier = m_definitions.count();

		m_identifiers[identifier] = Utils::createIdentifier(QLatin1String("CustomBar"), toolBars, false);

		definition.identifier = identifier;
		definition.location = Qt::TopToolBarArea;

		m_definitions.append(definition);

		emit m_instance->toolBarAdded(identifier);
	}
	else
	{
		const ToolBarsManager::ToolBarDefinition oldDefinition(m_definitions[identifier]);
		const bool wasMoved(definition.location != oldDefinition.location || definition.row != oldDefinition.row);

		m_definitions[identifier] = definition;

		if (wasMoved)
		{
			emit m_instance->toolBarMoved(identifier);
		}

		emit m_instance->toolBarModified(identifier);
	}

	m_instance->scheduleSave();
}

void ToolBarsManager::scheduleSave()
{
	if (!SessionsManager::isReadOnly() && m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

ToolBarsManager* ToolBarsManager::getInstance()
{
	return m_instance;
}

QString ToolBarsManager::getToolBarName(int identifier)
{
	ensureInitialized();

	if (identifier < OtherToolBar)
	{
		return EnumeratorMapper(staticMetaObject.enumerator(m_toolBarIdentifierEnumerator)).mapToName(identifier, false);
	}

	return m_identifiers.value(identifier);
}

QJsonValue ToolBarsManager::encodeEntry(const ToolBarDefinition::Entry &definition)
{
	if (definition.entries.isEmpty() && definition.options.isEmpty() && definition.parameters.isEmpty())
	{
		return {definition.action};
	}

	QJsonObject actionObject({{QLatin1String("identifier"), QJsonValue(definition.action)}});

	if (!definition.entries.isEmpty())
	{
		QJsonArray actionsArray;

		for (int i = 0; i < definition.entries.count(); ++i)
		{
			actionsArray.append(encodeEntry(definition.entries.at(i)));
		}

		actionObject.insert(QLatin1String("actions"), actionsArray);
	}

	if (!definition.options.isEmpty())
	{
		actionObject.insert(QLatin1String("options"), QJsonValue::fromVariant(definition.options));
	}

	if (!definition.parameters.isEmpty())
	{
		actionObject.insert(QLatin1String("parameters"), QJsonValue::fromVariant(definition.parameters));
	}

	return {actionObject};
}

ToolBarsManager::ToolBarDefinition::Entry ToolBarsManager::decodeEntry(const QJsonValue &value)
{
	ToolBarDefinition::Entry definition;

	if (value.isString())
	{
		definition.action = value.toString();

		return definition;
	}

	const QJsonObject actionObject(value.toObject());

	definition.action = actionObject.value(QLatin1String("identifier")).toString();
	definition.options = actionObject.value(QLatin1String("options")).toObject().toVariantMap();
	definition.parameters = actionObject.value(QLatin1String("parameters")).toObject().toVariantMap();

	if (actionObject.contains(QLatin1String("actions")))
	{
		const QJsonArray actionsArray(actionObject.value(QLatin1String("actions")).toArray());

		definition.entries.reserve(actionsArray.count());

		for (int i = 0; i < actionsArray.count(); ++i)
		{
			definition.entries.append(decodeEntry(actionsArray.at(i)));
		}
	}

	return definition;
}

ToolBarsManager::ToolBarDefinition ToolBarsManager::getToolBarDefinition(int identifier)
{
	ensureInitialized();

	if (identifier >= 0 && identifier < m_definitions.count())
	{
		ToolBarDefinition definition(m_definitions[identifier]);
		definition.identifier = identifier;

		return definition;
	}

	return {};
}

QHash<QString, ToolBarsManager::ToolBarDefinition> ToolBarsManager::loadToolBars(const QString &path, bool isDefault)
{
	QHash<QString, ToolBarsManager::ToolBarDefinition> definitions;
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		return definitions;
	}

	const QJsonArray toolBarsArray(QJsonDocument::fromJson(file.readAll()).array());
	const QMap<QString, ToolBarVisibility> visibilityModes({{QLatin1String("visible"), AlwaysVisibleToolBar}, {QLatin1String("hover"), OnHoverVisibleToolBar}, {QLatin1String("auto"), AutoVisibilityToolBar}, {QLatin1String("hidden"), AlwaysHiddenToolBar}});

	file.close();

	for (int i = 0; i < toolBarsArray.count(); ++i)
	{
		const QJsonObject toolBarObject(toolBarsArray.at(i).toObject());
		const QJsonArray actionsArray(toolBarObject.value(QLatin1String("actions")).toArray());
		const QString identifier(toolBarObject.value(QLatin1String("identifier")).toString());
		const QString location(toolBarObject.value(QLatin1String("location")).toString());
		const QString buttonStyle(toolBarObject.value(QLatin1String("buttonStyle")).toString());
		ToolBarDefinition definition;
		definition.title = toolBarObject.value(QLatin1String("title")).toString();
		definition.bookmarksPath = toolBarObject.value(QLatin1String("bookmarksPath")).toString();
		definition.currentPanel = toolBarObject.value(QLatin1String("currentPanel")).toString();
		definition.panels = toolBarObject.value(QLatin1String("panels")).toVariant().toStringList();
		definition.type = (toolBarObject.contains(QLatin1String("bookmarksPath")) ? BookmarksBarType : (toolBarObject.contains(QLatin1String("currentPanel")) ? SideBarType : ActionsBarType));
		definition.normalVisibility = visibilityModes.value(toolBarObject.value(QLatin1String("normalVisibility")).toString(), AlwaysVisibleToolBar);
		definition.fullScreenVisibility = visibilityModes.value(toolBarObject.value(QLatin1String("fullScreenVisibility")).toString(), AlwaysHiddenToolBar);
		definition.iconSize = toolBarObject.value(QLatin1String("iconSize")).toInt();
		definition.maximumButtonSize = toolBarObject.value(QLatin1String("maximumButtonSize")).toInt();
		definition.panelSize = toolBarObject.value(QLatin1String("panelSize")).toInt();
		definition.row = toolBarObject.value(QLatin1String("row")).toInt();
		definition.hasToggle = toolBarObject.value(QLatin1String("hasToggle")).toBool();
		definition.isDefault = isDefault;

		if (isDefault)
		{
			definition.identifier = getToolBarIdentifier(identifier);
		}

		if (definition.normalVisibility == OnHoverVisibleToolBar)
		{
			definition.normalVisibility = AlwaysVisibleToolBar;
		}

		if (location == QLatin1String("top"))
		{
			definition.location = Qt::TopToolBarArea;
		}
		else if (location == QLatin1String("bottom"))
		{
			definition.location = Qt::BottomToolBarArea;
		}
		else if (location == QLatin1String("left"))
		{
			definition.location = Qt::LeftToolBarArea;
		}
		else if (location == QLatin1String("right"))
		{
			definition.location = Qt::RightToolBarArea;
		}

		if (buttonStyle == QLatin1String("auto"))
		{
			definition.buttonStyle = Qt::ToolButtonFollowStyle;
		}
		else if (buttonStyle == QLatin1String("textOnly"))
		{
			definition.buttonStyle = Qt::ToolButtonTextOnly;
		}
		else if (buttonStyle == QLatin1String("textBesideIcon"))
		{
			definition.buttonStyle = Qt::ToolButtonTextBesideIcon;
		}
		else if (buttonStyle == QLatin1String("textUnderIcon"))
		{
			definition.buttonStyle = Qt::ToolButtonTextUnderIcon;
		}

		definition.entries.reserve(actionsArray.count());

		for (int j = 0; j < actionsArray.count(); ++j)
		{
			definition.entries.append(decodeEntry(actionsArray.at(j)));
		}

		definitions[identifier] = definition;
	}

	return definitions;
}

QVector<ToolBarsManager::ToolBarDefinition> ToolBarsManager::getToolBarDefinitions(Qt::ToolBarAreas areas)
{
	ensureInitialized();

	QVector<ToolBarsManager::ToolBarDefinition> definitions;
	definitions.reserve(m_definitions.count());

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		const ToolBarDefinition definition(m_definitions.at(i));

		if (!definition.wasRemoved && (areas == Qt::AllToolBarAreas || areas.testFlag(definition.location)))
		{
			definitions.append(definition);
		}
	}

	definitions.squeeze();

	return definitions;
}

int ToolBarsManager::getToolBarIdentifier(const QString &name)
{
	ensureInitialized();

	const int identifier(m_identifiers.key(name, -1));

	if (identifier >= 0)
	{
		return identifier;
	}

	return EnumeratorMapper(staticMetaObject.enumerator(m_toolBarIdentifierEnumerator)).mapToValue(name);
}

bool ToolBarsManager::areToolBarsLocked()
{
	return m_areToolBarsLocked;
}

}
