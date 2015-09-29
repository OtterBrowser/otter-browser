/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "SearchWidget.h"
#include "../MainWindow.h"
#include "../PreferencesDialog.h"
#include "../SearchDelegate.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/NotesManager.h"
#include "../../core/SearchesManager.h"
#include "../../core/SearchSuggester.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"
#include "../../core/Utils.h"

#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QLineEdit>

namespace Otter
{

SearchWidget::SearchWidget(Window *window, QWidget *parent) : QComboBox(parent),
	m_window(NULL),
	m_completer(new QCompleter(this)),
	m_suggester(NULL),
	m_lastValidIndex(0),
	m_isIgnoringActivation(false),
	m_isPopupUpdated(false),
	m_shouldSelectAllOnRelease(false),
	m_wasPopupVisible(false)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::PopupCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);

	setEditable(true);
	setMinimumWidth(100);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	setItemDelegate(new SearchDelegate(this));
	setModel(SearchesManager::getSearchEnginesModel());
	setInsertPolicy(QComboBox::NoInsert);
	optionChanged(QLatin1String("Search/SearchEnginesSuggestions"), SettingsManager::getValue(QLatin1String("Search/SearchEnginesSuggestions")));

	lineEdit()->setCompleter(m_completer);
	lineEdit()->setDragEnabled(true);
	lineEdit()->setStyleSheet(QLatin1String("QLineEdit {background:transparent;}"));
	lineEdit()->installEventFilter(this);

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}

	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(storeCurrentSearchEngine()));
	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModelModified()), this, SLOT(restoreCurrentSearchEngine()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(sendRequest(QString)));

	setWindow(window);
}

void SearchWidget::changeEvent(QEvent *event)
{
	QComboBox::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && itemData(currentIndex(), Qt::AccessibleDescriptionRole).toString().isEmpty())
	{
		lineEdit()->setPlaceholderText(tr("Search using %1").arg(currentData(Qt::UserRole).toString()));
	}
}

void SearchWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOptionFrame panel;
	panel.initFrom(lineEdit());
	panel.rect = rect();
	panel.palette = palette();
	panel.lineWidth = 1;

	style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

	if (isEnabled())
	{
		painter.drawPixmap(m_selectButtonIconRectangle, currentData(Qt::DecorationRole).value<QIcon>().pixmap(m_selectButtonIconRectangle.size()));

		QStyleOption arrow;
		arrow.initFrom(this);
		arrow.rect = m_selectButtonArrowRectangle;

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, this);
	}

	painter.drawPixmap(m_searchButtonRectangle, Utils::getIcon(QLatin1String("edit-find")).pixmap(m_searchButtonRectangle.size(), (isEnabled() ? QIcon::Active : QIcon::Disabled)));
}

void SearchWidget::resizeEvent(QResizeEvent *event)
{
	QComboBox::resizeEvent(event);

	QStyleOptionFrame panel;
	panel.initFrom(lineEdit());
	panel.rect = rect();
	panel.lineWidth = 1;

	const QRect rectangle = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);

	m_selectButtonIconRectangle = rectangle;
	m_selectButtonIconRectangle.setWidth(rectangle.height());
	m_selectButtonIconRectangle = m_selectButtonIconRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	m_selectButtonArrowRectangle = rectangle;
	m_selectButtonArrowRectangle.setLeft(rectangle.left() + rectangle.height());
	m_selectButtonArrowRectangle.setWidth(12);

	m_searchButtonRectangle = rectangle;
	m_searchButtonRectangle.setLeft(rectangle.right() - rectangle.height());
	m_searchButtonRectangle = m_searchButtonRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	lineEdit()->resize((rectangle.width() - m_selectButtonIconRectangle.width() - m_selectButtonArrowRectangle.width() - m_searchButtonRectangle.width()), rectangle.height());
	lineEdit()->move(m_selectButtonArrowRectangle.topRight());

	m_lineEditRectangle = lineEdit()->geometry();
}

void SearchWidget::focusInEvent(QFocusEvent *event)
{
	QComboBox::focusInEvent(event);

	activate(event->reason());
}

void SearchWidget::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && !(m_completer->popup() && m_completer->popup()->isVisible()))
	{
		const QString input = lineEdit()->text().trimmed();

		if (!input.isEmpty())
		{
			m_query = input;
		}

		if (!m_query.isEmpty())
		{
			emit requestedSearch(m_query, currentData(Qt::UserRole + 1).toString(), WindowsManager::calculateOpenHints(event->modifiers()));
		}
	}

	if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		disconnect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));

		m_isIgnoringActivation = true;
	}

	QComboBox::keyPressEvent(event);

	if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		m_isIgnoringActivation = false;

		lineEdit()->setText(m_query);

		connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	}
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(tr("Undo"), lineEdit(), SLOT(undo()), QKeySequence(QKeySequence::Undo))->setEnabled(lineEdit()->isUndoAvailable());
	menu.addAction(tr("Redo"), lineEdit(), SLOT(redo()), QKeySequence(QKeySequence::Redo))->setEnabled(lineEdit()->isRedoAvailable());
	menu.addSeparator();
	menu.addAction(tr("Cut"), lineEdit(), SLOT(cut()), QKeySequence(QKeySequence::Cut))->setEnabled(lineEdit()->hasSelectedText());
	menu.addAction(tr("Copy"), lineEdit(), SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(lineEdit()->hasSelectedText());
	menu.addAction(tr("Paste"), lineEdit(), SLOT(paste()), QKeySequence(QKeySequence::Paste))->setEnabled(!QApplication::clipboard()->text().isEmpty());
	menu.addAction(tr("Paste and Go"), this, SLOT(pasteAndGo()))->setEnabled(!QApplication::clipboard()->text().isEmpty());
	menu.addAction(tr("Delete"), this, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(lineEdit()->hasSelectedText());
	menu.addSeparator();
	menu.addAction(tr("Copy to Note"), this, SLOT(copyToNote()))->setEnabled(!lineEdit()->text().isEmpty());
	menu.addSeparator();
	menu.addAction(tr("Clear All"), lineEdit(), SLOT(clear()))->setEnabled(!lineEdit()->text().isEmpty());
	menu.addAction(tr("Select All"), lineEdit(), SLOT(selectAll()))->setEnabled(!lineEdit()->text().isEmpty());

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parentWidget());

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QList<QAction*>(), &menu));
	}

	menu.exec(event->globalPos());
}

void SearchWidget::mousePressEvent(QMouseEvent *event)
{
	m_wasPopupVisible = (m_popupHideTime.isValid() && m_popupHideTime.msecsTo(QTime::currentTime()) < 100);

	QWidget::mousePressEvent(event);
}

void SearchWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_selectButtonIconRectangle.contains(event->pos()) || m_selectButtonArrowRectangle.contains(event->pos()))
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
	}
	else if (m_searchButtonRectangle.contains(event->pos()))
	{
		sendRequest();
	}
	else
	{
		QWidget::mouseReleaseEvent(event);
	}
}

void SearchWidget::wheelEvent(QWheelEvent *event)
{
	disconnect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));

	m_isIgnoringActivation = true;

	QComboBox::wheelEvent(event);

	m_isIgnoringActivation = false;

	lineEdit()->setText(m_query);

	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
}

void SearchWidget::hidePopup()
{
	if (!m_query.isEmpty())
	{
		m_isPopupUpdated = true;
	}

	m_popupHideTime = QTime::currentTime();

	QComboBox::hidePopup();
}

void SearchWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Search/SearchEnginesSuggestions"))
	{
		if (value.toBool() && !m_suggester)
		{
			m_suggester = new SearchSuggester(getCurrentSearchEngine(), this);

			m_completer->setModel(m_suggester->getModel());

			connect(lineEdit(), SIGNAL(textEdited(QString)), m_suggester, SLOT(setQuery(QString)));
		}
		else if (!value.toBool() && m_suggester)
		{
			m_suggester->deleteLater();
			m_suggester = NULL;

			m_completer->setModel(NULL);
		}
	}
}

void SearchWidget::currentIndexChanged(int index)
{
	if (!m_storedSearchEngine.isEmpty())
	{
		return;
	}

	if (itemData(index, Qt::AccessibleDescriptionRole).toString().isEmpty())
	{
		m_lastValidIndex = index;

		SessionsManager::markSessionModified();

		emit searchEngineChanged(currentData(Qt::UserRole + 1).toString());

		lineEdit()->setPlaceholderText(tr("Search using %1").arg(itemData(index, Qt::UserRole).toString()));
		lineEdit()->setText(m_query);

		if (m_suggester)
		{
			m_suggester->setEngine(getCurrentSearchEngine());
			m_suggester->setQuery(QString());
		}

		if (!m_query.isEmpty() && sender() == this)
		{
			sendRequest();
		}
	}
	else
	{
		const QString query = m_query;

		setCurrentIndex(m_lastValidIndex);

		if (query != itemText(index))
		{
			lineEdit()->setText(query);
		}

		if (itemData(index, Qt::AccessibleDescriptionRole).toString() == QLatin1String("configure"))
		{
			PreferencesDialog dialog(QLatin1String("search"), this);
			dialog.exec();
		}
	}

	lineEdit()->setGeometry(m_lineEditRectangle);
}

void SearchWidget::queryChanged(const QString &query)
{
	if (m_isPopupUpdated)
	{
		m_isPopupUpdated = false;
	}
	else
	{
		m_query = query;
	}
}

void SearchWidget::sendRequest(const QString &query)
{
	if (!query.isEmpty())
	{
		m_query = query;
	}

	if (!m_query.isEmpty() && !m_isIgnoringActivation)
	{
		emit requestedSearch(m_query, currentData(Qt::UserRole + 1).toString(), WindowsManager::calculateOpenHints(QGuiApplication::keyboardModifiers()));
	}
}

void SearchWidget::copyToNote()
{
	const QString note(lineEdit()->hasSelectedText() ? lineEdit()->selectedText() : lineEdit()->text());

	if (!note.isEmpty())
	{
		NotesManager::addNote(BookmarksModel::UrlBookmark, QUrl(), note);
	}
}

void SearchWidget::deleteText()
{
	lineEdit()->del();
}

void SearchWidget::pasteAndGo()
{
	lineEdit()->paste();

	sendRequest();
}

void SearchWidget::storeCurrentSearchEngine()
{
	m_storedSearchEngine = getCurrentSearchEngine();

	hidePopup();

	disconnect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
	disconnect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
}

void SearchWidget::restoreCurrentSearchEngine()
{
	if (!m_storedSearchEngine.isEmpty())
	{
		setSearchEngine(m_storedSearchEngine);

		m_storedSearchEngine = QString();
	}

	lineEdit()->setText(m_query);
	lineEdit()->setGeometry(m_lineEditRectangle);

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
}

void SearchWidget::clearSelectAllOnRelease()
{
	if (m_shouldSelectAllOnRelease && lineEdit()->hasSelectedText())
	{
		disconnect(lineEdit(), SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));

		m_shouldSelectAllOnRelease = false;
	}
}

void SearchWidget::activate(Qt::FocusReason reason)
{
	if (!hasFocus() && isEnabled() && focusPolicy() != Qt::NoFocus)
	{
		setFocus(reason);

		return;
	}

	if (!lineEdit()->text().trimmed().isEmpty())
	{
		const bool selectAllOnFocus = SettingsManager::getValue(QLatin1String("AddressField/SelectAllOnFocus")).toBool();

		if (selectAllOnFocus && reason == Qt::MouseFocusReason)
		{
			m_shouldSelectAllOnRelease = true;

			connect(lineEdit(), SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));

			return;
		}

		if (selectAllOnFocus && (reason == Qt::ShortcutFocusReason || reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason))
		{
			QTimer::singleShot(0, lineEdit(), SLOT(selectAll()));
		}
		else if (reason != Qt::PopupFocusReason)
		{
			lineEdit()->deselect();
		}
	}
}

void SearchWidget::setSearchEngine(const QString &engine)
{
	const QStringList engines = SearchesManager::getSearchEngines();

	if (engines.isEmpty())
	{
		hidePopup();
		setEnabled(false);

		lineEdit()->setPlaceholderText(QString());

		return;
	}

	const int index = qMax(0, engines.indexOf(engine.isEmpty() ? SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString() : engine));

	if (index == currentIndex())
	{
		currentIndexChanged(currentIndex());
	}
	else
	{
		setCurrentIndex(index);
	}

	setEnabled(true);

	if (m_suggester)
	{
		m_suggester->setEngine(getCurrentSearchEngine());
	}
}

void SearchWidget::setWindow(Window *window)
{
	MainWindow *mainWindow = MainWindow::findMainWindow(this);

	if (m_window && (!sender() || sender() != m_window) && !m_window->isAboutToClose())
	{
		m_window->detachSearchWidget(this);

		disconnect(this, SIGNAL(requestedSearch(QString,QString,OpenHints)), m_window.data(), SIGNAL(requestedSearch(QString,QString,OpenHints)));
		disconnect(this, SIGNAL(searchEngineChanged(QString)), m_window.data(), SLOT(setSearchEngine(QString)));
		disconnect(m_window.data(), SIGNAL(searchEngineChanged(QString)), this, SLOT(setSearchEngine(QString)));
		disconnect(m_window.data(), SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));

		setSearchEngine();
	}

	m_window = window;

	if (window)
	{
		if (mainWindow)
		{
			disconnect(this, SIGNAL(requestedSearch(QString,QString,OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,OpenHints)));
		}

		window->attachSearchWidget(this);

		setSearchEngine(window->getSearchEngine());

		connect(this, SIGNAL(requestedSearch(QString,QString,OpenHints)), window, SIGNAL(requestedSearch(QString,QString,OpenHints)));
		connect(this, SIGNAL(searchEngineChanged(QString)), window, SLOT(setSearchEngine(QString)));
		connect(window, SIGNAL(searchEngineChanged(QString)), this, SLOT(setSearchEngine(QString)));
		connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
	}
	else
	{
		if (mainWindow)
		{
			connect(this, SIGNAL(requestedSearch(QString,QString,OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,OpenHints)));
		}

		setSearchEngine();
	}
}

QString SearchWidget::getCurrentSearchEngine() const
{
	return currentData(Qt::UserRole + 1).toString();
}

bool SearchWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == lineEdit() && event->type() == QEvent::Drop)
	{
		QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
		const QString dropAction = SettingsManager::getValue(QLatin1String("AddressField/DropAction")).toString();

		if (dropEvent && dropAction != QLatin1String("paste"))
		{
			if (dropAction == QLatin1String("pasteAndGo"))
			{
				sendRequest(dropEvent->mimeData()->text());
			}
			else if (dropAction == QLatin1String("replace"))
			{
				lineEdit()->setText(dropEvent->mimeData()->text());
			}

			event->accept();

			return true;
		}
	}
	else if (object == lineEdit() && event->type() == QEvent::MouseButtonRelease && m_shouldSelectAllOnRelease)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			disconnect(lineEdit(), SIGNAL(selectionChanged()), this, SLOT(clearSelectAllOnRelease()));

			lineEdit()->selectAll();

			m_shouldSelectAllOnRelease = false;
		}
	}

	return QObject::eventFilter(object, event);
}

}
