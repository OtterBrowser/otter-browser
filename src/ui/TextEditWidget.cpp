/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TextEditWidget.h"
#include "MainWindow.h"
#include "../core/NotesManager.h"
#include "../core/SpellCheckManager.h"
#ifdef OTTER_ENABLE_SPELLCHECK
#include "../../3rdparty/sonnet/src/ui/highlighter.h"
#endif

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>

namespace Otter
{

TextEditWidget::TextEditWidget(const QString &text, QWidget *parent) : QPlainTextEdit(text, parent), ActionExecutor(),
	m_hadSelection(false),
	m_wasEmpty(text.isEmpty())
{
	initialize();
}

TextEditWidget::TextEditWidget(QWidget *parent) : QPlainTextEdit(parent),
	m_hadSelection(false),
	m_wasEmpty(true)
{
	initialize();
}

void TextEditWidget::initialize()
{
	Sonnet::Highlighter *highlighter(new Sonnet::Highlighter(this));
	highlighter->setCurrentLanguage(SpellCheckManager::getDefaultDictionary());

	connect(this, &TextEditWidget::selectionChanged, this, &TextEditWidget::handleSelectionChanged);
	connect(this, &TextEditWidget::textChanged, this, &TextEditWidget::handleTextChanged);
	connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &TextEditWidget::notifyPasteActionStateChanged);
}

void TextEditWidget::focusInEvent(QFocusEvent *event)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow)
	{
		mainWindow->setActiveEditorExecutor(ActionExecutor::Object(this, this));
	}

	QPlainTextEdit::focusInEvent(event);
}

void TextEditWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
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
				const QString text(hasSelection() ? textCursor().selectedText() : toPlainText());

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
					const BookmarksItem *bookmark(NotesManager::getModel()->getBookmark(parameters[QLatin1String("note")].toULongLong()));

					if (bookmark)
					{
						insertPlainText(bookmark->getDescription());
					}
				}
				else if (parameters.contains(QLatin1String("text")))
				{
					insertPlainText(parameters[QLatin1String("text")].toString());
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
				insertPlainText({});
			}

			break;
		case ActionsManager::SelectAllAction:
			selectAll();

			break;
		case ActionsManager::UnselectAction:
			textCursor().setPosition(textCursor().position(), QTextCursor::MoveAnchor);

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

void TextEditWidget::handleSelectionChanged()
{
	if (hasSelection() != m_hadSelection)
	{
		m_hadSelection = hasSelection();

		emit actionsStateChanged(QVector<int>({ActionsManager::CutAction, ActionsManager::CopyAction, ActionsManager::CopyToNoteAction, ActionsManager::PasteAction, ActionsManager::DeleteAction, ActionsManager::UnselectAction}));
	}
}

void TextEditWidget::handleTextChanged()
{
	if (document() && document()->isEmpty() != m_wasEmpty)
	{
		m_wasEmpty = document()->isEmpty();

		emit actionsStateChanged(QVector<int>({ActionsManager::UndoAction, ActionsManager::RedoAction, ActionsManager::SelectAllAction, ActionsManager::ClearAllAction}));
	}
}

void TextEditWidget::notifyPasteActionStateChanged()
{
	emit actionsStateChanged(QVector<int>({ActionsManager::PasteAction}));
}

ActionsManager::ActionDefinition::State TextEditWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(identifier));
	ActionsManager::ActionDefinition::State state(definition.getDefaultState());
	state.isEnabled = false;

	if (definition.scope == ActionsManager::ActionDefinition::EditorScope)
	{
		switch (definition.identifier)
		{
			case ActionsManager::UndoAction:
				state.isEnabled = (!isReadOnly() && document() && document()->isUndoAvailable());

				break;
			case ActionsManager::RedoAction:
				state.isEnabled = (!isReadOnly() && document() && document()->isRedoAvailable());

				break;
			case ActionsManager::CutAction:
				state.isEnabled = (!isReadOnly() && hasSelection());

				break;
			case ActionsManager::CopyAction:
				state.isEnabled = hasSelection();

				break;
			case ActionsManager::CopyToNoteAction:
				state.isEnabled = hasSelection();

				break;
			case ActionsManager::PasteAction:
				state.isEnabled = (!isReadOnly() && (parameters.contains(QLatin1String("note")) || parameters.contains(QLatin1String("text")) || canPaste()));

				break;
			case ActionsManager::DeleteAction:
				state.isEnabled = (!isReadOnly() && hasSelection());

				break;
			case ActionsManager::SelectAllAction:
				state.isEnabled = (document() && !document()->isEmpty());

				break;
			case ActionsManager::UnselectAction:
				state.isEnabled = hasSelection();

				break;
			case ActionsManager::ClearAllAction:
				state.isEnabled = (!isReadOnly() && document() && !document()->isEmpty());

				break;
			default:
				break;
		}
	}

	return state;
}

bool TextEditWidget::hasSelection() const
{
	return textCursor().hasSelection();
}

}
