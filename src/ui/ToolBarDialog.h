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

#ifndef OTTER_TOOLBARDIALOG_H
#define OTTER_TOOLBARDIALOG_H

#include "Dialog.h"
#include "../core/ToolBarsManager.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class ToolBarDialog;
}

class ToolBarDialog : public Dialog
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		OptionsRole
	};

	explicit ToolBarDialog(int identifier = -1, QWidget *parent = NULL);
	~ToolBarDialog();

	ToolBarsManager::ToolBarDefinition getDefinition() const;
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void changeEvent(QEvent *event);
	void addEntry(const ActionsManager::ActionEntryDefinition &entry, QStandardItem *parent = NULL);
	QStandardItem* createEntry(const QString &identifier, const QVariantMap &options = QVariantMap());
	ActionsManager::ActionEntryDefinition getEntry(QStandardItem *item) const;

protected slots:
	void addEntry();
	void editEntry();
	void addBookmark(QAction *action);
	void restoreDefaults();
	void updateActions();

private:
	ToolBarsManager::ToolBarDefinition m_definition;
	Ui::ToolBarDialog *m_ui;
};

}

#endif
