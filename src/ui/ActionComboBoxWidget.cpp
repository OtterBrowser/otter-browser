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

#include "ActionComboBoxWidget.h"
#include "ItemViewWidget.h"
#include "LineEditWidget.h"
#include "../core/ItemModel.h"

#include <QtCore/QTimer>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ActionComboBoxWidget::ActionComboBoxWidget(QWidget *parent) : ComboBoxWidget(parent),
	m_filterLineEditWidget(nullptr),
	m_model(new QStandardItemModel(this)),
	m_wasPopupVisible(false)
{
	setEditable(true);

	lineEdit()->hide();

	getView()->viewport()->parentWidget()->installEventFilter(this);

	const QVector<ActionsManager::ActionDefinition> definitions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		const ActionsManager::ActionDefinition definition(definitions.at(i));

		if (!definition.flags.testFlag(ActionsManager::ActionDefinition::IsDeprecatedFlag) && !definition.flags.testFlag(ActionsManager::ActionDefinition::RequiresParameters))
		{
			addDefinition(definition);
		}
	}

	m_model->setSortRole(IdentifierRole);

	setModel(m_model);
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
	m_popupHideTime = {};

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

void ActionComboBoxWidget::addDefinition(const ActionsManager::ActionDefinition &definition)
{
	const QString name(ActionsManager::getActionName(definition.identifier));
	QStandardItem *item(new QStandardItem(definition.getText(true)));
	item->setData(ItemModel::createDecoration(definition.defaultState.icon), Qt::DecorationRole);
	item->setData(definition.identifier, IdentifierRole);
	item->setData(name, NameRole);
	item->setToolTip(QStringLiteral("%1 (%2)").arg(item->text(), name));
	item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

	m_model->appendRow(item);
}

void ActionComboBoxWidget::setActionIdentifier(int action)
{
	const int index(findData(action));

	if (index >= 0)
	{
		setCurrentIndex(index);

		return;
	}

	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(action));

	if (definition.isValid())
	{
		addDefinition(definition);
		setCurrentIndex(count() - 1);

		m_model->sort(0);
	}
}

int ActionComboBoxWidget::getActionIdentifier() const
{
	return ((currentIndex() >= 0) ? currentData(IdentifierRole).toInt() : -1);
}

bool ActionComboBoxWidget::eventFilter(QObject *object, QEvent *event)
{
	switch (event->type())
	{
		case QEvent::Hide:
			if (m_filterLineEditWidget)
			{
				m_filterLineEditWidget->clear();
			}

			break;
		case QEvent::Move:
		case QEvent::Resize:
		case QEvent::Show:
			if (!m_filterLineEditWidget)
			{
				ItemViewWidget *view(getView());

				m_filterLineEditWidget = new LineEditWidget(view->viewport()->parentWidget());
				m_filterLineEditWidget->setClearButtonEnabled(true);
				m_filterLineEditWidget->setPlaceholderText(tr("Search…"));

				view->setFilterRoles({Qt::DisplayRole, NameRole});
				view->setStyleSheet(QStringLiteral("QAbstractItemView {padding:0 0 %1px 0;}").arg(m_filterLineEditWidget->height()));

				connect(m_filterLineEditWidget, &LineEditWidget::textChanged, view, &ItemViewWidget::setFilterString);
			}

			if (event->type() == QEvent::Show)
			{
				QTimer::singleShot(0, m_filterLineEditWidget, static_cast<void(LineEditWidget::*)()>(&LineEditWidget::setFocus));
			}
			else
			{
				ItemViewWidget *view(getView());

				m_filterLineEditWidget->resize(view->width(), m_filterLineEditWidget->height());
				m_filterLineEditWidget->move(0, (view->height() - m_filterLineEditWidget->height()));
			}

			break;
		default:
			break;
	}

	return QObject::eventFilter(object, event);
}

}
