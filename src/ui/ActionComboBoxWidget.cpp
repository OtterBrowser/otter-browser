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

#include "ActionComboBoxWidget.h"
#include "ItemViewWidget.h"
#include "../core/ActionsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtWidgets/QLineEdit>

namespace Otter
{

ActionComboBoxWidget::ActionComboBoxWidget(QWidget *parent) : QComboBox(parent),
	m_view(new ItemViewWidget(this)),
	m_filterLineEdit(NULL),
	m_wasPopupVisible(false)
{
	setEditable(true);
	setView(m_view);

	m_view->setHeaderHidden(true);

	lineEdit()->hide();

	view()->viewport()->parentWidget()->installEventFilter(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	const QVector<ActionsManager::ActionDefinition> definitions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(definitions.at(i).icon, QCoreApplication::translate("actions", (definitions.at(i).description.isEmpty() ? definitions.at(i).text : definitions.at(i).description).toUtf8().constData())));
		item->setData(definitions.at(i).identifier, Qt::UserRole);
		item->setToolTip(ActionsManager::getActionName(definitions.at(i).identifier));
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		model->appendRow(item);
	}

	setModel(model);
	setCurrentIndex(-1);
}

void ActionComboBoxWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QStyleOptionComboBox comboBoxOption;
	comboBoxOption.initFrom(this);
	comboBoxOption.editable = false;

	style()->drawComplexControl(QStyle::CC_ComboBox, &comboBoxOption, &painter, this);

	QStyleOptionViewItem viewItemOption;
	viewItemOption.initFrom(this);
	viewItemOption.displayAlignment |= Qt::AlignVCenter;

	if (currentIndex() >= 0)
	{
		itemDelegate()->paint(&painter, viewItemOption, m_view->getSourceModel()->index(currentIndex(), 0));
	}
	else
	{
		const int textMargin(style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, this) + 1);

		painter.setFont(viewItemOption.font);
		painter.drawText(viewItemOption.rect.adjusted(textMargin, 0, -textMargin, 0), Qt::AlignVCenter, tr("Select Action"));
	}
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
	setCurrentIndex(findData(action));
}

int ActionComboBoxWidget::getActionIdentifier() const
{
	return m_view->getSourceModel()->index(currentIndex(), 0).data(Qt::UserRole).toInt();
}

bool ActionComboBoxWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::Show || event->type() == QEvent::Move || event->type() == QEvent::Resize)
	{
		if (!m_filterLineEdit)
		{
			m_filterLineEdit = new QLineEdit(view()->viewport()->parentWidget());
			m_filterLineEdit->setClearButtonEnabled(true);
			m_filterLineEdit->setPlaceholderText(tr("Search…"));

			view()->setStyleSheet(QStringLiteral("QAbstractItemView {padding:0 0 %1px 0;}").arg(m_filterLineEdit->height()));

			connect(m_filterLineEdit, SIGNAL(textChanged(QString)), m_view, SLOT(setFilterString(QString)));
		}

		if (event->type() == QEvent::Show)
		{
			QTimer::singleShot(0, m_filterLineEdit, SLOT(setFocus()));
		}
		else
		{
			m_filterLineEdit->resize(view()->width(), m_filterLineEdit->height());
			m_filterLineEdit->move(0, (view()->height() - m_filterLineEdit->height()));
		}
	}
	else if (event->type() == QEvent::Hide && m_filterLineEdit)
	{
		m_filterLineEdit->clear();
	}

	return QObject::eventFilter(object, event);
}

}
