/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Action.h"
#include "MainWindow.h"
#include "Menu.h"
#include "../core/NotesManager.h"
#include "../core/SpellCheckManager.h"
#ifdef OTTER_ENABLE_SPELLCHECK
#include "../../3rdparty/sonnet/src/ui/highlighter.h"
#endif

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

namespace Otter
{

TextEditWidget::TextEditWidget(const QString &text, QWidget *parent) : QPlainTextEdit(text, parent),
	m_highlighter(nullptr),
	m_isSpellCheckingEnabled(true),
	m_hadSelection(false),
	m_wasEmpty(text.isEmpty())
{
	initialize();
}

TextEditWidget::TextEditWidget(bool isSpellCheckingEnabled, QWidget *parent) : QPlainTextEdit(parent),
	m_highlighter(nullptr),
	m_isSpellCheckingEnabled(isSpellCheckingEnabled),
	m_hadSelection(false),
	m_wasEmpty(true)
{
	initialize();
}

TextEditWidget::TextEditWidget(QWidget *parent) : QPlainTextEdit(parent),
	m_highlighter(nullptr),
	m_isSpellCheckingEnabled(true),
	m_hadSelection(false),
	m_wasEmpty(true)
{
	initialize();
}

void TextEditWidget::initialize()
{
#ifdef OTTER_ENABLE_SPELLCHECK
	if (m_isSpellCheckingEnabled)
	{
		m_highlighter = new Sonnet::Highlighter(this);
		m_highlighter->setCurrentLanguage(SpellCheckManager::getDefaultDictionary());
	}
#endif

	connect(this, &TextEditWidget::selectionChanged, this, [&]()
	{
		if (hasSelection() != m_hadSelection)
		{
			m_hadSelection = hasSelection();

			emit arbitraryActionsStateChanged({ActionsManager::CutAction, ActionsManager::CopyAction, ActionsManager::CopyToNoteAction, ActionsManager::PasteAction, ActionsManager::DeleteAction, ActionsManager::UnselectAction});
		}
	});
	connect(this, &TextEditWidget::textChanged, this, [&]()
	{
		if (document() && document()->isEmpty() != m_wasEmpty)
		{
			m_wasEmpty = document()->isEmpty();

			emit arbitraryActionsStateChanged({ActionsManager::UndoAction, ActionsManager::RedoAction, ActionsManager::SelectAllAction, ActionsManager::ClearAllAction});
		}
	});
	connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, [&]()
	{
		emit arbitraryActionsStateChanged({ActionsManager::PasteAction});
	});
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

void TextEditWidget::contextMenuEvent(QContextMenuEvent *event)
{
	ActionExecutor::Object executor(this, this);
	QMenu menu(this);
	Menu *notesMenu(new Menu(Menu::NotesMenu, &menu));
	notesMenu->setExecutor(executor);

	menu.addMenu(notesMenu);
	menu.addSeparator();
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
	menu.addSeparator();
	menu.addAction(new Action(ActionsManager::CheckSpellingAction, {}, executor, &menu));

	Menu *dictionariesMenu(new Menu(Menu::DictionariesMenu, &menu));
	dictionariesMenu->setExecutor(executor);

	menu.addMenu(dictionariesMenu);
	menu.exec(event->globalPos());
}

void TextEditWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
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
					const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(parameters[QLatin1String("note")].toULongLong()));

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
				textCursor().removeSelectedText();
			}

			break;
		case ActionsManager::SelectAllAction:
			selectAll();

			break;
		case ActionsManager::UnselectAction:
			textCursor().clearSelection();

			break;
		case ActionsManager::ClearAllAction:
			if (!isReadOnly())
			{
				selectAll();
				textCursor().removeSelectedText();
			}

			break;
		case ActionsManager::CheckSpellingAction:
#ifdef OTTER_ENABLE_SPELLCHECK
			if (!m_isSpellCheckingEnabled)
			{
				return;
			}

			if (parameters.contains(QLatin1String("dictionary")))
			{
				const QString dictionary(parameters[QLatin1String("dictionary")].toString());

				SettingsManager::setOption(SettingsManager::Browser_SpellCheckDictionaryOption, dictionary);

				if (!m_highlighter)
				{
					m_highlighter = new Sonnet::Highlighter(this);
				}

				m_highlighter->setCurrentLanguage(dictionary);
				m_highlighter->setActive(true);
				m_highlighter->slotRehighlight();
			}
			else
			{
				const bool isEnabled(parameters.value(QLatin1String("isChecked"), !(m_highlighter && m_highlighter->isActive())).toBool());

				if (isEnabled && !m_highlighter)
				{
					m_highlighter = new Sonnet::Highlighter(this);
					m_highlighter->setCurrentLanguage(SpellCheckManager::getDefaultDictionary());
				}
				else if (!isEnabled && m_highlighter)
				{
					m_highlighter->deleteLater();
					m_highlighter = nullptr;
				}
			}
#endif
			break;
		default:
			break;
	}
}

void TextEditWidget::setSpellCheckingEnabled(bool isSpellCheckingEnabled)
{
	m_isSpellCheckingEnabled = isSpellCheckingEnabled;

#ifdef OTTER_ENABLE_SPELLCHECK
	if (!isSpellCheckingEnabled && m_highlighter)
	{
		m_highlighter->deleteLater();
		m_highlighter = nullptr;
	}
#endif

	emit arbitraryActionsStateChanged({ActionsManager::CheckSpellingAction});
}

ActionsManager::ActionDefinition::State TextEditWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(identifier));
	ActionsManager::ActionDefinition::State state(definition.getDefaultState());
	state.isEnabled = false;

	if (definition.scope != ActionsManager::ActionDefinition::EditorScope)
	{
		return state;
	}

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
		case ActionsManager::CheckSpellingAction:
#ifdef OTTER_ENABLE_SPELLCHECK
			{
				state.isEnabled = m_isSpellCheckingEnabled;

				if (parameters.contains(QLatin1String("dictionary")))
				{
					const QString language(parameters[QLatin1String("dictionary")].toString());
					const SpellCheckManager::DictionaryInformation dictionary(SpellCheckManager::getDictionary(language));

					state.text = (dictionary.isValid() ? dictionary.title : language);
					state.isChecked = (language == (SettingsManager::getOption(SettingsManager::Browser_SpellCheckDictionaryOption).isNull() ? SpellCheckManager::getDefaultDictionary() : SettingsManager::getOption(SettingsManager::Browser_SpellCheckDictionaryOption).toString()));
				}
				else
				{
					state.isChecked = (m_highlighter && m_highlighter->isActive());
				}
			}
#else
			state.isEnabled = false;
#endif
			break;
		default:
			break;
	}

	return state;
}

bool TextEditWidget::isSpellCheckingEnabled() const
{
#ifdef OTTER_ENABLE_SPELLCHECK
	return (m_highlighter && m_highlighter->isActive());
#else
	return false;
#endif
}

bool TextEditWidget::hasSelection() const
{
	return textCursor().hasSelection();
}

}
