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

#ifndef OTTER_ACTION_H
#define OTTER_ACTION_H

#include "../core/ActionExecutor.h"

#if QT_VERSION >= 0x060000
#include <QtGui/QAction>
#else
#include <QtWidgets/QAction>
#endif

namespace Otter
{

class Action : public QAction
{
	Q_OBJECT

public:
	enum ActionFlag
	{
		NoFlags = 0,
		HasIconOverrideFlag = 1,
		HasTextOverrideFlag = 2,
		IsTextOverrideTranslateableFlag = 4
	};

	Q_DECLARE_FLAGS(ActionFlags, ActionFlag)

	explicit Action(const QString &text, bool isTranslateable, QObject *parent);
	explicit Action(int identifier, const QVariantMap &parameters, QObject *parent);
	explicit Action(int identifier, const QVariantMap &parameters, const ActionExecutor::Object &executor, QObject *parent);

	void setExecutor(ActionExecutor::Object executor);
	void setIconOverride(const QString &icon);
	void setIconOverride(const QIcon &icon);
	void setTextOverride(const QString &text, bool isTranslateable = true);
	QString getTextOverride() const;
	ActionsManager::ActionDefinition getDefinition() const;
	QVariantMap getParameters() const;
	int getIdentifier() const;
	bool hasIconOverride() const;
	bool hasTextOverride() const;
	bool isTextOverrideTranslateable() const;
	bool event(QEvent *event) override;

protected:
	void initialize();
	void updateIcon();
	virtual void setState(const ActionsManager::ActionDefinition::State &state);
	QMetaMethod getMethod(const char *method) const;

protected slots:
	void triggerAction(bool isChecked = false);
	void handleArbitraryActionsStateChanged(const QVector<int> &identifiers);
	void handleCategorizedActionsStateChanged(const QVector<int> &categories);
	void updateShortcut();
	void updateState();

private:
	ActionExecutor::Object m_executor;
	QString m_textOverride;
	QVariantMap m_parameters;
	ActionFlags m_flags;
	int m_identifier;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::Action::ActionFlags)

#endif
