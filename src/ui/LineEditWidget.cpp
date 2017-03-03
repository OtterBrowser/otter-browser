/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "LineEditWidget.h"
#include "../core/NotesManager.h"

#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

namespace Otter
{

PopupViewWidget::PopupViewWidget(LineEditWidget *parent) : ItemViewWidget(nullptr),
	m_lineEditWidget(parent)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setFocusPolicy(Qt::NoFocus);
	setWindowFlags(Qt::Popup);
	header()->setStretchLastSection(true);
	header()->hide();
	viewport()->setAttribute(Qt::WA_Hover);
	viewport()->setMouseTracking(true);
	viewport()->installEventFilter(this);

	connect(this, SIGNAL(needsActionsUpdate()), this, SLOT(updateHeight()));
	connect(this, SIGNAL(entered(QModelIndex)), this, SLOT(handleIndexEntered(QModelIndex)));
}

void PopupViewWidget::keyPressEvent(QKeyEvent *event)
{
	if (!m_lineEditWidget)
	{
		ItemViewWidget::keyPressEvent(event);

		return;
	}

	m_lineEditWidget->event(event);

	if (event->isAccepted())
	{
		if (!m_lineEditWidget->hasFocus())
		{
			m_lineEditWidget->hidePopup();
		}

		return;
	}

	switch (event->key())
	{
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_End:
			ItemViewWidget::keyPressEvent(event);

			return;
		case Qt::Key_Enter:
		case Qt::Key_Return:
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
		case Qt::Key_Escape:
		case Qt::Key_F4:
			if (event->key() == Qt::Key_F4 && !event->modifiers().testFlag(Qt::AltModifier))
			{
				break;
			}

			if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
			{
				emit clicked(getCurrentIndex());
			}

			m_lineEditWidget->hidePopup();

			break;
		default:
			break;
	}
}

void PopupViewWidget::handleIndexEntered(const QModelIndex &index)
{
	if (index.isValid())
	{
		setCurrentIndex(index);
	}

	QStatusTipEvent statusTipEvent(index.data(Qt::StatusTipRole).toString());

	QApplication::sendEvent(m_lineEditWidget, &statusTipEvent);
}

void PopupViewWidget::updateHeight()
{
	int completionHeight(5);
	const int rowCount(qMin(20, getRowCount()));

	for (int i = 0; i < rowCount; ++i)
	{
		completionHeight += sizeHintForRow(i);
	}

	setFixedHeight(completionHeight);
	viewport()->setFixedHeight(completionHeight - 3);
}

bool PopupViewWidget::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::Close:
		case QEvent::Hide:
		case QEvent::Leave:
			if (m_lineEditWidget)
			{
				QString statusTip;
				QStatusTipEvent statusTipEvent(statusTip);

				QApplication::sendEvent(m_lineEditWidget, &statusTipEvent);
			}

			break;
		case QEvent::InputMethod:
		case QEvent::ShortcutOverride:
			if (m_lineEditWidget)
			{
				QApplication::sendEvent(m_lineEditWidget, event);
			}

			break;
		case QEvent::MouseButtonPress:
			if (m_lineEditWidget && !viewport()->underMouse())
			{
				m_lineEditWidget->hidePopup();
			}

			break;
		default:
			break;
	}

	return ItemViewWidget::event(event);
}

LineEditWidget::LineEditWidget(QWidget *parent) : QLineEdit(parent),
	m_popupViewWidget(nullptr),
	m_dropMode(PasteDropMode),
	m_selectionStart(-1),
	m_shouldIgnoreCompletion(false),
	m_shouldSelectAllOnFocus(false),
	m_shouldSelectAllOnRelease(false)
{
	setDragEnabled(true);
}

LineEditWidget::~LineEditWidget()
{
	if (m_popupViewWidget)
	{
		m_popupViewWidget->deleteLater();
	}
}

void LineEditWidget::resizeEvent(QResizeEvent *event)
{
	QLineEdit::resizeEvent(event);

	if (m_popupViewWidget)
	{
		m_popupViewWidget->move(mapToGlobal(contentsRect().bottomLeft()));
		m_popupViewWidget->setFixedWidth(width());
	}
}

void LineEditWidget::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) && !m_completion.isEmpty())
	{
		m_shouldIgnoreCompletion = true;
	}

	QLineEdit::keyPressEvent(event);
}

void LineEditWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_selectionStart = selectionStart();
	}

	QLineEdit::mousePressEvent(event);
}

void LineEditWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_shouldSelectAllOnRelease)
	{
		disconnect(this, SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));

		selectAll();

		m_shouldSelectAllOnRelease = false;
	}

	QLineEdit::mouseReleaseEvent(event);
}

void LineEditWidget::dropEvent(QDropEvent *event)
{
	if (m_selectionStart >= 0)
	{
		const int selectionEnd(m_selectionStart + event->mimeData()->text().length());
		int dropPosition(cursorPositionAt(event->pos()));

		if (dropPosition < m_selectionStart || dropPosition > selectionEnd)
		{
			if (dropPosition > selectionEnd)
			{
				dropPosition -= event->mimeData()->text().length();
			}

			setSelection(m_selectionStart, event->mimeData()->text().length());
			del();
			setCursorPosition(dropPosition);
			insert(event->mimeData()->text());
		}
	}
	else if (m_dropMode == ReplaceDropMode || m_dropMode == ReplaceAndNotifyDropMode)
	{
		selectAll();
		del();
		insert(event->mimeData()->text());

		if (m_dropMode == ReplaceAndNotifyDropMode)
		{
			emit textDropped(event->mimeData()->text());
		}
	}
	else
	{
		QLineEdit::dropEvent(event);
	}
}

void LineEditWidget::clearSelectAllOnRelease()
{
	if (m_shouldSelectAllOnRelease && hasSelectedText())
	{
		disconnect(this, SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));

		m_shouldSelectAllOnRelease = false;
	}
}

void LineEditWidget::activate(Qt::FocusReason reason)
{
	if (!hasFocus() && isEnabled() && focusPolicy() != Qt::NoFocus)
	{
		setFocus(reason);

		return;
	}

	if (!text().trimmed().isEmpty())
	{
		if (m_shouldSelectAllOnFocus && reason == Qt::MouseFocusReason)
		{
			m_shouldSelectAllOnRelease = true;

			connect(this, SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));

			return;
		}

		if (m_shouldSelectAllOnFocus && (reason == Qt::ShortcutFocusReason || reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason))
		{
			QTimer::singleShot(0, this, SLOT(selectAll()));
		}
		else if (reason != Qt::PopupFocusReason)
		{
			deselect();
		}
	}
}

void LineEditWidget::copyToNote()
{
	const QString note(hasSelectedText() ? selectedText() : text());

	if (!note.isEmpty())
	{
		NotesManager::addNote(BookmarksModel::UrlBookmark, QUrl(), note);
	}
}

void LineEditWidget::deleteText()
{
	del();
}

void LineEditWidget::showPopup()
{
	if (!m_popupViewWidget)
	{
		m_popupViewWidget = new PopupViewWidget(this);
	}

	m_popupViewWidget->updateHeight();
	m_popupViewWidget->move(mapToGlobal(contentsRect().bottomLeft()));
	m_popupViewWidget->setFixedWidth(width());
	m_popupViewWidget->setFocusProxy(this);
	m_popupViewWidget->show();
}

void LineEditWidget::hidePopup()
{
	if (m_popupViewWidget)
	{
		m_popupViewWidget->hide();
		m_popupViewWidget->deleteLater();
		m_popupViewWidget = nullptr;

		QString statusTip;
		QStatusTipEvent statusTipEvent(statusTip);

		QApplication::sendEvent(this, &statusTipEvent);
	}
}

void LineEditWidget::setCompletion(const QString &completion)
{
	m_completion = completion;

	if (m_shouldIgnoreCompletion)
	{
		m_shouldIgnoreCompletion = false;

		return;
	}

	QString currentText(text().mid(selectionStart()));

	if (m_completion != currentText)
	{
		setText(m_completion);
		setCursorPosition(currentText.length());
		setSelection(currentText.length(), (m_completion.length() - currentText.length()));
	}
}

void LineEditWidget::setDropMode(LineEditWidget::DropMode mode)
{
	m_dropMode = mode;
}

void LineEditWidget::setSelectAllOnFocus(bool select)
{
	if (m_shouldSelectAllOnFocus && !select)
	{
		disconnect(this, SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));
	}
	else if (!m_shouldSelectAllOnFocus && select)
	{
		connect(this, SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));
	}

	m_shouldSelectAllOnFocus = select;
}

PopupViewWidget* LineEditWidget::getPopup()
{
	if (!m_popupViewWidget)
	{
		m_popupViewWidget = new PopupViewWidget(this);
	}

	return m_popupViewWidget;
}

bool LineEditWidget::isPopupVisible() const
{
	return (m_popupViewWidget && m_popupViewWidget->isVisible());
}

}
