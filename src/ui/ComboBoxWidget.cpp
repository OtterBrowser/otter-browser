/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ComboBoxWidget.h"
#include "ItemViewWidget.h"

namespace Otter
{

ComboBoxWidget::ComboBoxWidget(QWidget *parent) : QComboBox(parent),
	m_view(new ItemViewWidget(this))
{
	m_view->header()->setStretchLastSection(true);
	m_view->setViewMode(ItemViewWidget::TreeView);
	m_view->setHeaderHidden(true);
	m_view->setItemsExpandable(false);
	m_view->setRootIsDecorated(false);
	m_view->setStyleSheet(QLatin1String("QTreeView::branch {border-image:url(invalid.png);}"));

	setView(m_view);
}

void ComboBoxWidget::setCurrentIndex(const QModelIndex &index)
{
	if (!index.isValid())
	{
		QComboBox::setCurrentIndex(-1);

		return;
	}

	setRootModelIndex(index.parent());
	setModelColumn(0);
	setCurrentIndex(index.row());
	setRootModelIndex(QModelIndex());
}

void ComboBoxWidget::setCurrentIndex(int index)
{
	QComboBox::setCurrentIndex(index);
}

ItemViewWidget* ComboBoxWidget::getView() const
{
	return m_view;
}

bool ComboBoxWidget::event(QEvent *event)
{
	if (event->type() == QEvent::DynamicPropertyChange)
	{
		m_view->expandAll();
	}

	return QComboBox::event(event);
}

}
