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

#ifndef OTTER_CONSOLEWIDGET_H
#define OTTER_CONSOLEWIDGET_H

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QWidget>

#include "../../../core/Console.h"

namespace Otter
{

namespace Ui
{
	class ErrorConsoleWidget;
}

class ErrorConsoleWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit ErrorConsoleWidget(QWidget *parent = nullptr);
	~ErrorConsoleWidget();

protected:
	enum DataRole
	{
		TimeRole = Qt::UserRole,
		CategoryRole,
		SourceRole,
		WindowRole
	};

	enum MessagesScope
	{
		NoScope = 0,
		CurrentTabScope = 1,
		AllTabsScope = 2,
		OtherSourcesScope = 4
	};

	Q_DECLARE_FLAGS(MessagesScopes, MessagesScope)

	void showEvent(QShowEvent *event) override;
	void applyFilters(const QString &filter, const QVector<Console::MessageCategory> &categories, quint64 activeWindow);
	void applyFilters(const QModelIndex &index, const QString &filter, const QVector<Console::MessageCategory> &categories, quint64 activeWindow);
	QVector<Console::MessageCategory> getCategories() const;
	quint64 getActiveWindow();

protected slots:
	void addMessage(const Console::Message &message);
	void filterCategories();
	void filterMessages(const QString &filter);
	void showContextMenu(const QPoint &position);

private:
	QStandardItemModel *m_model;
	MessagesScopes m_messageScopes;
	Ui::ErrorConsoleWidget *m_ui;
};

}

#endif
