/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TEXTEDITWIDGET_H
#define OTTER_TEXTEDITWIDGET_H

#include "../core/ActionExecutor.h"

#include <QtWidgets/QPlainTextEdit>

namespace Sonnet
{
	class Highlighter;
}

namespace Otter
{

class TextEditWidget final : public QPlainTextEdit, public ActionExecutor
{
	Q_OBJECT

public:
	explicit TextEditWidget(const QString &text, QWidget *parent = nullptr);
	explicit TextEditWidget(QWidget *parent = nullptr);

	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters) const override;
	bool hasSelection() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void focusInEvent(QFocusEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void initialize();

protected slots:
	void handleSelectionChanged();
	void handleTextChanged();
	void notifyPasteActionStateChanged();

private:
	Sonnet::Highlighter *m_highlighter;
	bool m_hadSelection;
	bool m_wasEmpty;

signals:
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
};

}

#endif
