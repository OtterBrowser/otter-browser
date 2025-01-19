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

#ifndef OTTER_ACTIONCOMBOBOXWIDGET_H
#define OTTER_ACTIONCOMBOBOXWIDGET_H

#include "ComboBoxWidget.h"
#include "../core/ActionsManager.h"

#include <QtCore/QTime>
#include <QtGui/QMouseEvent>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class LineEditWidget;

class ActionComboBoxWidget final : public ComboBoxWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		NameRole
	};

	explicit ActionComboBoxWidget(QWidget *parent = nullptr);

	void setActionIdentifier(int action);
	int getActionIdentifier() const;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void addDefinition(const ActionsManager::ActionDefinition &definition);

private:
	LineEditWidget *m_filterLineEditWidget;
	QStandardItemModel *m_model;
	QTime m_popupHideTime;
	bool m_wasPopupVisible;
};

}

#endif
