/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "Action.h"
#include "MainWindow.h"
#include "ToolBarWidget.h"
#include "../core/NotesManager.h"

#include <QtCore/QMimeData>
#include <QtCore/QStringListModel>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>

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

	connect(this, &PopupViewWidget::needsActionsUpdate, this, &PopupViewWidget::updateHeight);
	connect(this, &PopupViewWidget::entered, this, &PopupViewWidget::handleIndexEntered);
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
				const QModelIndex index(getCurrentIndex());

				if (index.isValid())
				{
					emit clicked(index);
				}
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

LineEditWidget::LineEditWidget(const QString &text, QWidget *parent) : QLineEdit(text, parent), ActionExecutor(),
	m_popupViewWidget(nullptr),
	m_completer(nullptr),
	m_dropMode(PasteDropMode),
	m_selectionStart(-1),
	m_shouldClearOnEscape(false),
	m_shouldIgnoreCompletion(false),
	m_shouldSelectAllOnFocus(false),
	m_shouldSelectAllOnRelease(false),
	m_hadSelection(false),
	m_wasEmpty(text.isEmpty())
{
	initialize();
}

LineEditWidget::LineEditWidget(QWidget *parent) : QLineEdit(parent),
	m_popupViewWidget(nullptr),
	m_completer(nullptr),
	m_dropMode(PasteDropMode),
	m_selectionStart(-1),
	m_shouldClearOnEscape(false),
	m_shouldIgnoreCompletion(false),
	m_shouldSelectAllOnFocus(false),
	m_shouldSelectAllOnRelease(false),
	m_hadSelection(false),
	m_wasEmpty(true)
{
	initialize();
}

LineEditWidget::~LineEditWidget()
{
	if (m_popupViewWidget)
	{
		m_popupViewWidget->deleteLater();
	}
}

void LineEditWidget::initialize()
{
	setDragEnabled(true);

	connect(this, &LineEditWidget::selectionChanged, this, &LineEditWidget::handleSelectionChanged);
	connect(this, &LineEditWidget::textChanged, this, &LineEditWidget::handleTextChanged);
	connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &LineEditWidget::notifyPasteActionStateChanged);
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

void LineEditWidget::focusInEvent(QFocusEvent *event)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow)
	{
		mainWindow->setActiveEditorExecutor(ActionExecutor::Object(this, this));
	}

	QLineEdit::focusInEvent(event);
}

void LineEditWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Backspace:
		case Qt::Key_Delete:
			if (!m_completion.isEmpty())
			{
				m_shouldIgnoreCompletion = true;
			}

			break;
		case Qt::Key_Escape:
			if (m_shouldClearOnEscape)
			{
				clear();
			}

			break;
		default:
			break;
	}

	QLineEdit::keyPressEvent(event);
}

void LineEditWidget::contextMenuEvent(QContextMenuEvent *event)
{
	ActionExecutor::Object executor(this, this);
	QMenu menu(this);
	menu.addAction(new Action(ActionsManager::UndoAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::RedoAction, {}, executor, &menu));
	menu.addSeparator();
	menu.addAction(new Action(ActionsManager::CutAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::CopyAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::PasteAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::DeleteAction, {}, executor, &menu));
	menu.addSeparator();
	menu.addAction(new Action(ActionsManager::CopyToNoteAction, {}, executor, &menu));
	menu.addSeparator();
	menu.addAction(new Action(ActionsManager::ClearAllAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::SelectAllAction, {}, executor, &menu));

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), {}, &menu));
	}

	menu.exec(event->globalPos());
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
		disconnect(this, &LineEditWidget::selectionChanged, this, &LineEditWidget::clearSelectAllOnRelease);

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
		clear();
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

void LineEditWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	Q_UNUSED(trigger)

	switch (identifier)
	{
		case ActionsManager::UndoAction:
			if (!isReadOnly())
			{
				undo();
			}

			break;
		case ActionsManager::RedoAction:
			if (!isReadOnly())
			{
				redo();
			}

			break;
		case ActionsManager::CutAction:
			if (!isReadOnly())
			{
				cut();
			}

			break;
		case ActionsManager::CopyAction:
			copy();

			break;
		case ActionsManager::CopyToNoteAction:
			{
				const QString text(hasSelectedText() ? selectedText() : this->text());

				if (!text.isEmpty())
				{
					NotesManager::addNote(BookmarksModel::UrlBookmark, {{BookmarksModel::DescriptionRole, text}});
				}
			}

			break;
		case ActionsManager::PasteAction:
			if (!isReadOnly())
			{
				if (parameters.contains(QLatin1String("note")))
				{
					const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(parameters[QLatin1String("note")].toULongLong()));

					if (bookmark)
					{
						insert(bookmark->getDescription());
					}
				}
				else if (parameters.contains(QLatin1String("text")))
				{
					insert(parameters[QLatin1String("text")].toString());
				}
				else
				{
					paste();
				}
			}

			break;
		case ActionsManager::DeleteAction:
			if (!isReadOnly())
			{
				del();
			}

			break;
		case ActionsManager::SelectAllAction:
			selectAll();

			break;
		case ActionsManager::UnselectAction:
			deselect();

			break;
		case ActionsManager::ClearAllAction:
			if (!isReadOnly())
			{
				clear();
			}

			break;
		default:
			break;
	}
}

void LineEditWidget::clearSelectAllOnRelease()
{
	if (m_shouldSelectAllOnRelease && hasSelectedText())
	{
		disconnect(this, &LineEditWidget::selectionChanged, this, &LineEditWidget::clearSelectAllOnRelease);

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

			connect(this, &LineEditWidget::selectionChanged, this, &LineEditWidget::clearSelectAllOnRelease);

			return;
		}

		if (m_shouldSelectAllOnFocus && (reason == Qt::ShortcutFocusReason || reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason))
		{
			QTimer::singleShot(0, this, &LineEditWidget::selectAll);
		}
		else if (reason != Qt::PopupFocusReason)
		{
			deselect();
		}
	}
}

void LineEditWidget::showPopup()
{
	if (!m_popupViewWidget)
	{
		getPopup();
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

void LineEditWidget::handleSelectionChanged()
{
	if (hasSelectedText() != m_hadSelection)
	{
		m_hadSelection = hasSelectedText();

		emit arbitraryActionsStateChanged({ActionsManager::CutAction, ActionsManager::CopyAction, ActionsManager::CopyToNoteAction, ActionsManager::PasteAction, ActionsManager::DeleteAction, ActionsManager::UnselectAction});
	}
}

void LineEditWidget::handleTextChanged(const QString &text)
{
	if (text.isEmpty() != m_wasEmpty)
	{
		m_wasEmpty = text.isEmpty();

		emit arbitraryActionsStateChanged({ActionsManager::UndoAction, ActionsManager::RedoAction, ActionsManager::SelectAllAction, ActionsManager::ClearAllAction});
	}
}

void LineEditWidget::notifyPasteActionStateChanged()
{
	emit arbitraryActionsStateChanged({ActionsManager::PasteAction});
}

void LineEditWidget::setCompletion(const QString &completion)
{
	if (completion != m_completion)
	{
		m_completion = completion;

		if (m_shouldIgnoreCompletion)
		{
			m_shouldIgnoreCompletion = false;

			return;
		}

		if (!m_completer)
		{
			m_completer = new QCompleter(this);
			m_completer->setCompletionMode(QCompleter::InlineCompletion);

			setCompleter(m_completer);
		}

		m_completer->setModel(new QStringListModel({completion}, m_completer));
		m_completer->complete();
	}
}

void LineEditWidget::setDropMode(LineEditWidget::DropMode mode)
{
	m_dropMode = mode;
}

void LineEditWidget::setClearOnEscape(bool clear)
{
	m_shouldClearOnEscape = clear;
}

void LineEditWidget::setSelectAllOnFocus(bool select)
{
	if (m_shouldSelectAllOnFocus && !select)
	{
		disconnect(this, &LineEditWidget::selectionChanged, this, &LineEditWidget::clearSelectAllOnRelease);
	}
	else if (!m_shouldSelectAllOnFocus && select)
	{
		connect(this, &LineEditWidget::selectionChanged, this, &LineEditWidget::clearSelectAllOnRelease);
	}

	m_shouldSelectAllOnFocus = select;
}

PopupViewWidget* LineEditWidget::getPopup()
{
	if (!m_popupViewWidget)
	{
		m_popupViewWidget = new PopupViewWidget(this);

		connect(m_popupViewWidget, &PopupViewWidget::clicked, this, &LineEditWidget::popupClicked);
	}

	return m_popupViewWidget;
}

ActionsManager::ActionDefinition::State LineEditWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(identifier));
	ActionsManager::ActionDefinition::State state(definition.getDefaultState());
	state.isEnabled = false;

	if (definition.scope == ActionsManager::ActionDefinition::EditorScope)
	{
		switch (definition.identifier)
		{
			case ActionsManager::UndoAction:
				state.isEnabled = (!isReadOnly() && isUndoAvailable());

				break;
			case ActionsManager::RedoAction:
				state.isEnabled = (!isReadOnly() && isRedoAvailable());

				break;
			case ActionsManager::CutAction:
				state.isEnabled = (!isReadOnly() && hasSelectedText());

				break;
			case ActionsManager::CopyAction:
				state.isEnabled = hasSelectedText();

				break;
			case ActionsManager::CopyToNoteAction:
				state.isEnabled = hasSelectedText();

				break;
			case ActionsManager::PasteAction:
				state.isEnabled = (!isReadOnly() && (parameters.contains(QLatin1String("note")) || parameters.contains(QLatin1String("text")) || (QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText())));

				break;
			case ActionsManager::DeleteAction:
				state.isEnabled = (!isReadOnly() && hasSelectedText());

				break;
			case ActionsManager::SelectAllAction:
				state.isEnabled = !text().isEmpty();

				break;
			case ActionsManager::UnselectAction:
				state.isEnabled = hasSelectedText();

				break;
			case ActionsManager::ClearAllAction:
				state.isEnabled = (!isReadOnly() && !text().isEmpty());

				break;
			default:
				break;
		}
	}

	return state;
}

bool LineEditWidget::isPopupVisible() const
{
	return (m_popupViewWidget && m_popupViewWidget->isVisible());
}

}
