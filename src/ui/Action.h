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

#ifndef OTTER_ACTION_H
#define OTTER_ACTION_H

#include "../core/ActionExecutor.h"

#include <QtWidgets/QAction>

namespace Otter
{

class Action final : public QAction
{
	Q_OBJECT

public:
	enum ActionFlag
	{
		NoFlags = 0,
		IsOverridingTextFlag = 1,
		IsOverridingIconFlag = 2
	};

	Q_DECLARE_FLAGS(ActionFlags, ActionFlag)

	explicit Action(int identifier, const QVariantMap &parameters, QObject *parent);
	explicit Action(int identifier, const QVariantMap &parameters, ActionExecutor::Object executor, QObject *parent);
	explicit Action(int identifier, const QVariantMap &parameters, const QVariantMap &options, ActionExecutor::Object executor, QObject *parent);

	void setExecutor(ActionExecutor::Object executor);
	ActionsManager::ActionDefinition getDefinition() const;
	QVariantMap getParameters() const;
	int getIdentifier() const;
	bool event(QEvent *event) override;

protected:
	void initialize();
	void updateIcon();
	void setState(const ActionsManager::ActionDefinition::State &state);
	ActionsManager::ActionDefinition::State getState() const;

protected slots:
	void triggerAction(bool isChecked = false);
	void handleArbitraryActionsStateChanged(const QVector<int> &identifiers);
	void handleCategorizedActionsStateChanged(const QVector<int> &categories);
	void updateShortcut();
	void updateState();

private:
	ActionExecutor::Object m_executor;
	QString m_overrideText;
	QVariantMap m_parameters;
	ActionFlags m_flags;
	int m_identifier;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::Action::ActionFlags)

#endif
