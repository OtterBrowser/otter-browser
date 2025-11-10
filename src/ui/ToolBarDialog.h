/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

class OptionWidget;

class ToolBarDialog final : public Dialog
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		OptionsRole,
		ParametersRole,
		HasOptionsRole
	};

	explicit ToolBarDialog(const ToolBarsManager::ToolBarDefinition &definition = {}, QWidget *parent = nullptr);
	~ToolBarDialog();

	ToolBarsManager::ToolBarDefinition getDefinition() const;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void changeEvent(QEvent *event) override;
	void addEntry(const ToolBarsManager::ToolBarDefinition::Entry &entry, QStandardItem *parent = nullptr);
	QStandardItem* createEntry(const QString &identifier, const QVariantMap &options = {}, const QVariantMap &parameters = {}) const;
	ToolBarsManager::ToolBarDefinition::Entry getEntry(QStandardItem *item) const;
	QMap<int, QVariant> createEntryData(const QString &identifier, const QVariantMap &options = {}, const QVariantMap &parameters = {}) const;

protected slots:
	void addNewEntry();
	void editEntry();
	void addBookmark(QAction *action);
	void restoreDefaults();
	void showAvailableEntriesContextMenu(const QPoint &position);
	void showCurrentEntriesContextMenu(const QPoint &position);
	void updateActions();

private:
	QStandardItemModel *m_currentEntriesModel;
	ToolBarsManager::ToolBarDefinition m_definition;
	Ui::ToolBarDialog *m_ui;
};

}

#endif
