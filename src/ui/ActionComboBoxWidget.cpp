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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ActionComboBoxWidget.h"
#include "ItemViewWidget.h"
#include "../core/ActionsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ActionComboBoxWidget::ActionComboBoxWidget(QWidget *parent) : ComboBoxWidget(parent),
	m_filterLineEdit(nullptr),
	m_wasPopupVisible(false)
{
	setEditable(true);

	lineEdit()->hide();

	getView()->viewport()->parentWidget()->installEventFilter(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	const QVector<ActionsManager::ActionDefinition> definitions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(definitions.at(i).getText(true)));
		item->setData(QColor(Qt::transparent), Qt::DecorationRole);
		item->setData(definitions.at(i).identifier, Qt::UserRole);
		item->setToolTip(ActionsManager::getActionName(definitions.at(i).identifier));
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		if (!definitions.at(i).defaultState.icon.isNull())
		{
			item->setIcon(definitions.at(i).defaultState.icon);
		}

		model->appendRow(item);
	}

	setModel(model);
	setCurrentIndex(-1);
}

void ActionComboBoxWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionComboBox comboBoxOption;

	initStyleOption(&comboBoxOption);

	comboBoxOption.editable = false;

	if (currentIndex() < 0)
	{
		comboBoxOption.currentText = tr("Select Action");
	}

	if (comboBoxOption.currentIcon.isNull())
	{
		QPixmap pixmap(1, 1);
		pixmap.fill(Qt::transparent);

		comboBoxOption.currentIcon = QIcon(pixmap);
	}

	painter.drawComplexControl(QStyle::CC_ComboBox, comboBoxOption);
	painter.drawControl(QStyle::CE_ComboBoxLabel, comboBoxOption);
}

void ActionComboBoxWidget::mousePressEvent(QMouseEvent *event)
{
	m_wasPopupVisible = (m_popupHideTime.isValid() && m_popupHideTime.msecsTo(QTime::currentTime()) < 100);

	QWidget::mousePressEvent(event);
}

void ActionComboBoxWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_popupHideTime = QTime();

	if (m_wasPopupVisible)
	{
		hidePopup();
	}
	else
	{
		showPopup();
	}

	QWidget::mouseReleaseEvent(event);
}

void ActionComboBoxWidget::setActionIdentifier(int action)
{
	const int index(findData(action));

	if (index >= 0)
	{
		setCurrentIndex(index);
	}
}

int ActionComboBoxWidget::getActionIdentifier() const
{
	return ((currentIndex() >= 0) ? currentData(Qt::UserRole).toInt() : -1);
}

bool ActionComboBoxWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::Show || event->type() == QEvent::Move || event->type() == QEvent::Resize)
	{
		if (!m_filterLineEdit)
		{
			m_filterLineEdit = new QLineEdit(getView()->viewport()->parentWidget());
			m_filterLineEdit->setClearButtonEnabled(true);
			m_filterLineEdit->setPlaceholderText(tr("Search…"));

			getView()->setStyleSheet(QStringLiteral("QAbstractItemView {padding:0 0 %1px 0;}").arg(m_filterLineEdit->height()));

			connect(m_filterLineEdit, SIGNAL(textChanged(QString)), getView(), SLOT(setFilterString(QString)));
		}

		if (event->type() == QEvent::Show)
		{
			QTimer::singleShot(0, m_filterLineEdit, SLOT(setFocus()));
		}
		else
		{
			m_filterLineEdit->resize(getView()->width(), m_filterLineEdit->height());
			m_filterLineEdit->move(0, (getView()->height() - m_filterLineEdit->height()));
		}
	}
	else if (event->type() == QEvent::Hide && m_filterLineEdit)
	{
		m_filterLineEdit->clear();
	}

	return QObject::eventFilter(object, event);
}

}
