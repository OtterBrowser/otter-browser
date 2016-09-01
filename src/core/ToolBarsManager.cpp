/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtCore/QMetaEnum>
#include <QtCore/QVector>
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ToolBarsManager* ToolBarsManager::m_instance = NULL;
QMap<int, QString> ToolBarsManager::m_identifiers;
QVector<ToolBarsManager::ToolBarDefinition> ToolBarsManager::m_definitions;
int ToolBarsManager::m_toolBarIdentifierEnumerator = 0;
bool ToolBarsManager::m_areToolBarsLocked = false;

ToolBarsManager::ToolBarsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Menu Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Tab Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Navigation Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Progress Bar"))
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Status Bar"))
}

void ToolBarsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ToolBarsManager(parent);
		m_toolBarIdentifierEnumerator = m_instance->metaObject()->indexOfEnumerator(QLatin1String("ToolBarIdentifier").data());
		m_areToolBarsLocked = SettingsManager::getValue(SettingsManager::Interface_LockToolBarsOption).toBool();
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
			const QMap<ToolBarVisibility, QString> visibilityModes({{AlwaysVisibleToolBar, QLatin1String("visible")}, {OnHoverVisibleToolBar, QLatin1String("hover")}, {AutoVisibilityToolBar, QLatin1String("auto")}, {AlwaysHiddenToolBar, QLatin1String("hidden")}});

			for (int i = 0; i < m_definitions.count(); ++i)
			{
				if ((m_definitions[i].isDefault && !m_definitions[i].canReset) || m_definitions[i].wasRemoved)
				{
					continue;
				}

				QString identifier(getToolBarName(m_definitions[i].identifier));

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

				definition.insert(QLatin1String("normalVisibility"), QJsonValue(visibilityModes.value(m_definitions[i].normalVisibility)));
				definition.insert(QLatin1String("fullScreenVisibility"), QJsonValue(visibilityModes.value(m_definitions[i].fullScreenVisibility)));

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
					definition.insert(QLatin1String("maximumButtonSize"), QJsonValue(m_definitions[i].maximumButtonSize));
				}

				definition.insert(QLatin1String("row"), QJsonValue(m_definitions[i].row));

				if (!m_definitions[i].entries.isEmpty())
				{
					QJsonArray actions;

					for (int j = 0; j < m_definitions[i].entries.count(); ++j)
					{
						actions.append(encodeEntry(m_definitions[i].entries.at(j)));
					}

					definition.insert(QLatin1String("actions"), actions);
				}

				definitions.append(definition);
			}

			QJsonDocument document;
			document.setArray(definitions);

			QFile file(SessionsManager::getWritableDataPath(QLatin1String("toolBars.json")));

			if (file.open(QIODevice::WriteOnly))
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
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action && identifier < 0)
	{
		identifier = action->data().toInt();
	}

	if (identifier >= 0 && identifier < m_definitions.count())
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
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action && identifier < 0)
	{
		identifier = action->data().toInt();
	}

	if (identifier >= 0 && identifier < OtherToolBar && QMessageBox::question(NULL, tr("Reset Toolbar"), tr("Do you really want to reset this toolbar to default configuration?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Yes)
	{
		const QHash<QString, ToolBarDefinition> defaultDefinitions(loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true));
		ToolBarDefinition definition(defaultDefinitions.value(getToolBarName(identifier)));
		definition.identifier = identifier;

		setToolBar(definition);
	}
}

void ToolBarsManager::removeToolBar(int identifier)
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action && identifier < 0)
	{
		identifier = action->data().toInt();
	}

	if (identifier >= 0 && identifier < m_definitions.count() && identifier >= OtherToolBar && QMessageBox::question(NULL, tr("Remove Toolbar"), tr("Do you really want to remove this toolbar?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Yes)
	{
		m_definitions[identifier].wasRemoved = true;
		m_definitions[identifier].title = QString();
		m_definitions[identifier].entries.clear();

		m_instance->scheduleSave();

		emit m_instance->toolBarRemoved(identifier);
	}
}

void ToolBarsManager::resetToolBars()
{
	if (QMessageBox::question(NULL, tr("Reset Toolbars"), tr("Do you really want to reset all toolbars to default configuration?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel)
	{
		return;
	}

	const QList<int> customToolBars(m_identifiers.keys());

	for (int i = 0; i < customToolBars.count(); ++i)
	{
		emit toolBarRemoved(customToolBars.at(i));
	}

	m_definitions.clear();
	m_identifiers.clear();

	const QHash<QString, ToolBarDefinition> defaultDefinitions(loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true));

	m_definitions.append(defaultDefinitions[QLatin1String("MenuBar")]);
	m_definitions.append(defaultDefinitions[QLatin1String("TabBar")]);
	m_definitions.append(defaultDefinitions[QLatin1String("NavigationBar")]);
	m_definitions.append(defaultDefinitions[QLatin1String("ProgressBar")]);
	m_definitions.append(defaultDefinitions[QLatin1String("StatusBar")]);

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		emit toolBarModified(i);
	}

	m_instance->scheduleSave();
}

void ToolBarsManager::setToolBar(ToolBarsManager::ToolBarDefinition definition)
{
	int identifier(definition.identifier);

	if (identifier < 0 || identifier >= m_definitions.count())
	{
		QStringList toolBars({QLatin1String("MenuBar"), QLatin1String("TabBar"), QLatin1String("NavigationBar"), QLatin1String("ProgressBar"), QLatin1String("StatusBar")});
		toolBars.append(m_identifiers.values());

		identifier = m_definitions.count();

		m_identifiers[identifier] = Utils::createIdentifier(QLatin1String("CustomBar"), toolBars, false);

		m_definitions.append(definition);
		m_definitions[identifier].identifier = identifier;
		m_definitions[identifier].location = Qt::TopToolBarArea;

		emit m_instance->toolBarAdded(identifier);
	}
	else
	{
		if (identifier < OtherToolBar)
		{
			definition.canReset = true;
		}

		m_definitions[identifier] = definition;

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

void ToolBarsManager::setToolBarsLocked(bool locked)
{
	SettingsManager::setValue(SettingsManager::Interface_LockToolBarsOption, locked);

	emit m_instance->toolBarsLockedChanged(locked);
}

ToolBarsManager* ToolBarsManager::getInstance()
{
	return m_instance;
}

QString ToolBarsManager::getToolBarName(int identifier)
{
	if (identifier < OtherToolBar)
	{
		return m_instance->metaObject()->enumerator(m_toolBarIdentifierEnumerator).valueToKey(identifier);
	}

	return m_identifiers.value(identifier);
}

QJsonValue ToolBarsManager::encodeEntry(const ActionsManager::ActionEntryDefinition &definition)
{
	if (definition.entries.isEmpty() && definition.options.isEmpty())
	{
		return QJsonValue(definition.action);
	}

	QJsonObject action;
	action.insert(QLatin1String("identifier"), QJsonValue(definition.action));

	if (!definition.entries.isEmpty())
	{
		QJsonArray actions;

		for (int i = 0; i < definition.entries.count(); ++i)
		{
			actions.append(encodeEntry(definition.entries.at(i)));
		}

		action.insert(QLatin1String("actions"), actions);
	}

	if (!definition.options.isEmpty())
	{
		QJsonObject options;
		QVariantMap::const_iterator optionsIterator;

		for (optionsIterator = definition.options.begin(); optionsIterator != definition.options.end(); ++optionsIterator)
		{
			options.insert(optionsIterator.key(), QJsonValue::fromVariant(optionsIterator.value()));
		}

		action.insert(QLatin1String("options"), options);
	}

	return QJsonValue(action);
}

ActionsManager::ActionEntryDefinition ToolBarsManager::decodeEntry(const QJsonValue &value)
{
	ActionsManager::ActionEntryDefinition definition;

	if (value.isString())
	{
		definition.action = value.toString();

		return definition;
	}

	const QJsonObject object(value.toObject());

	definition.action = object.value(QLatin1String("identifier")).toString();
	definition.options = object.value(QLatin1String("options")).toObject().toVariantMap();

	if (object.contains(QLatin1String("actions")))
	{
		const QJsonArray actions(object.value(QLatin1String("actions")).toArray());

		for (int i = 0; i < actions.count(); ++i)
		{
			definition.entries.append(decodeEntry(actions.at(i)));
		}
	}

	return definition;
}

ToolBarsManager::ToolBarDefinition ToolBarsManager::getToolBarDefinition(int identifier)
{
	if (m_definitions.isEmpty())
	{
		getToolBarDefinitions();
	}

	if (identifier >= 0 && identifier < m_definitions.count())
	{
		ToolBarDefinition definition(m_definitions[identifier]);
		definition.identifier = identifier;

		return definition;
	}

	return ToolBarDefinition();
}

QHash<QString, ToolBarsManager::ToolBarDefinition> ToolBarsManager::loadToolBars(const QString &path, bool isDefault)
{
	QHash<QString, ToolBarsManager::ToolBarDefinition> definitions;
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		return definitions;
	}

	const QJsonArray toolBars(QJsonDocument::fromJson(file.readAll()).array());
	const QMap<QString, ToolBarVisibility> visibilityModes({{QLatin1String("visible"), AlwaysVisibleToolBar}, {QLatin1String("hover"), OnHoverVisibleToolBar}, {QLatin1String("auto"), AutoVisibilityToolBar}, {QLatin1String("hidden"), AlwaysHiddenToolBar}});

	file.close();

	for (int i = 0; i < toolBars.count(); ++i)
	{
		const QJsonObject toolBarObject(toolBars.at(i).toObject());
		const QJsonArray actions(toolBarObject.value(QLatin1String("actions")).toArray());
		const QString identifier(toolBarObject.value(QLatin1String("identifier")).toString());
		const QString location(toolBarObject.value(QLatin1String("location")).toString());
		const QString buttonStyle(toolBarObject.value(QLatin1String("buttonStyle")).toString());
		ToolBarDefinition toolBar;
		toolBar.title = toolBarObject.value(QLatin1String("title")).toString();
		toolBar.bookmarksPath = toolBarObject.value(QLatin1String("bookmarksPath")).toString();
		toolBar.normalVisibility = visibilityModes.value(toolBarObject.value(QLatin1String("normalVisibility")).toString(), AlwaysVisibleToolBar);
		toolBar.fullScreenVisibility = visibilityModes.value(toolBarObject.value(QLatin1String("fullScreenVisibility")).toString(), AlwaysHiddenToolBar);
		toolBar.iconSize = toolBarObject.value(QLatin1String("iconSize")).toInt();
		toolBar.maximumButtonSize = toolBarObject.value(QLatin1String("maximumButtonSize")).toInt();
		toolBar.row = toolBarObject.value(QLatin1String("row")).toInt();
		toolBar.isDefault = isDefault;

		if (toolBar.normalVisibility == OnHoverVisibleToolBar)
		{
			toolBar.normalVisibility = AlwaysVisibleToolBar;
		}

		if (isDefault)
		{
			toolBar.title = QCoreApplication::translate("actions", toolBar.title.toUtf8());
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
			toolBar.entries.append(decodeEntry(actions.at(j)));
		}

		definitions[identifier] = toolBar;
	}

	return definitions;
}

QVector<ToolBarsManager::ToolBarDefinition> ToolBarsManager::getToolBarDefinitions()
{
	if (m_definitions.isEmpty())
	{
		const QHash<QString, ToolBarsManager::ToolBarDefinition> defaultDefinitions(loadToolBars(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json"), true), true));

		m_definitions.reserve(OtherToolBar);

		for (int i = 0; i < OtherToolBar; ++i)
		{
			m_definitions.append(defaultDefinitions.value(getToolBarName(i)));
		}

		const QString customToolBarsPath(SessionsManager::getReadableDataPath(QLatin1String("toolBars.json")));

		if (QFile::exists(customToolBarsPath))
		{
			const QHash<QString, ToolBarDefinition> customDefinitions(loadToolBars(customToolBarsPath, false));

			if (!customDefinitions.isEmpty())
			{
				QHash<QString, ToolBarDefinition>::const_iterator iterator;

				for (iterator = customDefinitions.constBegin(); iterator != customDefinitions.constEnd(); ++iterator)
				{
					int identifier(getToolBarIdentifier(iterator.key()));

					if (identifier >= 0)
					{
						m_definitions[identifier] = iterator.value();
						m_definitions[identifier].canReset = true;
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

		bool hasMenuBar(false);

		for (int i = 0; i < m_definitions[MenuBar].entries.count(); ++i)
		{
			if (m_definitions[MenuBar].entries.at(i).action == QLatin1String("MenuBarWidget"))
			{
				hasMenuBar = true;

				break;
			}
		}

		if (!hasMenuBar)
		{
			ActionsManager::ActionEntryDefinition definition;
			definition.action = QLatin1String("MenuBarWidget");

			m_definitions[MenuBar].entries.prepend(definition);
		}

		bool hasTabBar(false);

		for (int i = 0; i < m_definitions[TabBar].entries.count(); ++i)
		{
			if (m_definitions[TabBar].entries.at(i).action == QLatin1String("TabBarWidget"))
			{
				hasTabBar = true;

				break;
			}
		}

		if (!hasTabBar)
		{
			ActionsManager::ActionEntryDefinition definition;
			definition.action = QLatin1String("TabBarWidget");

			m_definitions[TabBar].entries.prepend(definition);
		}
	}

	QVector<ToolBarsManager::ToolBarDefinition> definitions;
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

int ToolBarsManager::getToolBarIdentifier(const QString &name)
{
	const int identifier(m_identifiers.key(name, -1));

	if (identifier >= 0)
	{
		return identifier;
	}

	return m_instance->metaObject()->enumerator(m_toolBarIdentifierEnumerator).keyToValue(name.toLatin1());
}

bool ToolBarsManager::areToolBarsLocked()
{
	return m_areToolBarsLocked;
}

}
