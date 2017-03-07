/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "SearchWidget.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/SearchSuggester.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/PreferencesDialog.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QClipboard>
#include <QtGui/QPainter>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolTip>

namespace Otter
{

SearchDelegate::SearchDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void SearchDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		QStyleOptionFrame frameOption;
		frameOption.palette = option.palette;
		frameOption.rect = option.rect;
		frameOption.state = option.state;
		frameOption.frameShape = QFrame::HLine;

		QApplication::style()->drawControl(QStyle::CE_ShapedFrame, &frameOption, painter, 0);

		return;
	}

	QRect titleRectangle(option.rect);

	if (index.data(Qt::DecorationRole).value<QIcon>().isNull())
	{
		drawDisplay(painter, option, titleRectangle, index.data(Qt::DisplayRole).toString());

		return;
	}

	QRect decorationRectangle(option.rect);

	if (option.direction == Qt::RightToLeft)
	{
		decorationRectangle.setLeft(option.rect.width() - option.rect.height());
	}
	else
	{
		decorationRectangle.setRight(option.rect.height());
	}

	decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	index.data(Qt::DecorationRole).value<QIcon>().paint(painter, decorationRectangle, option.decorationAlignment);

	if (option.direction == Qt::RightToLeft)
	{
		titleRectangle.setRight(option.rect.width() - option.rect.height());
	}
	else
	{
		titleRectangle.setLeft(option.rect.height());
	}

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("configure"))
	{
		drawDisplay(painter, option, titleRectangle, index.data(Qt::DisplayRole).toString());

		return;
	}

	const int shortcutWidth((option.rect.width() > 150) ? 40 : 0);

	if (shortcutWidth > 0)
	{
		QRect shortcutReactangle(option.rect);

		if (option.direction == Qt::RightToLeft)
		{
			shortcutReactangle.setRight(shortcutWidth);

			titleRectangle.setLeft(shortcutWidth + 5);
		}
		else
		{
			shortcutReactangle.setLeft(option.rect.right() - shortcutWidth);

			titleRectangle.setRight(titleRectangle.right() - (shortcutWidth + 5));
		}

		drawDisplay(painter, option, shortcutReactangle, index.data(SearchEnginesManager::KeywordRole).toString());
	}

	drawDisplay(painter, option, titleRectangle, index.data(SearchEnginesManager::TitleRole).toString());
}

QSize SearchDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		size.setHeight(option.fontMetrics.height() * 0.75);
	}
	else
	{
		size.setHeight(option.fontMetrics.height() * 1.25);
	}

	return size;
}

SearchWidget::SearchWidget(Window *window, QWidget *parent) : LineEditWidget(parent),
	m_window(nullptr),
	m_suggester(nullptr),
	m_isIgnoringActivation(false),
	m_isSearchEngineLocked(false)
{
	setMinimumWidth(100);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	handleOptionChanged(SettingsManager::AddressField_DropActionOption, SettingsManager::getOption(SettingsManager::AddressField_DropActionOption));
	handleOptionChanged(SettingsManager::AddressField_SelectAllOnFocusOption, SettingsManager::getOption(SettingsManager::AddressField_SelectAllOnFocusOption));
	handleOptionChanged(SettingsManager::Search_SearchEnginesSuggestionsOption, SettingsManager::getOption(SettingsManager::Search_SearchEnginesSuggestionsOption));

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}

	connect(SearchEnginesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(storeCurrentSearchEngine()));
	connect(SearchEnginesManager::getInstance(), SIGNAL(searchEnginesModelModified()), this, SLOT(restoreCurrentSearchEngine()));
	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
	connect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));
	connect(this, SIGNAL(textDropped(QString)), this, SLOT(sendRequest(QString)));

	setWindow(window);
}

void SearchWidget::changeEvent(QEvent *event)
{
	LineEditWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			{
				const QString title(SearchEnginesManager::getSearchEngine(m_storedSearchEngine).title);

				setToolTip(tr("Search using %1").arg(title));
				setPlaceholderText(tr("Search using %1").arg(title));
			}

			break;
		case QEvent::LayoutDirectionChange:
			updateGeometries();

			break;
		default:
			break;
	}
}

void SearchWidget::paintEvent(QPaintEvent *event)
{
	LineEditWidget::paintEvent(event);

	QPainter painter(this);

	if (isEnabled())
	{
		painter.drawPixmap(m_iconRectangle, SearchEnginesManager::getSearchEngine(m_storedSearchEngine).icon.pixmap(m_iconRectangle.size()));

		QStyleOption arrow;
		arrow.initFrom(this);
		arrow.rect = m_dropdownArrowRectangle;

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, this);
	}

	if (m_addButtonRectangle.isValid())
	{
		painter.drawPixmap(m_addButtonRectangle, ThemesManager::getIcon(QLatin1String("list-add")).pixmap(m_addButtonRectangle.size(), (isEnabled() ? QIcon::Active : QIcon::Disabled)));
	}

	if (m_searchButtonRectangle.isValid())
	{
		painter.drawPixmap(m_searchButtonRectangle, ThemesManager::getIcon(QLatin1String("edit-find")).pixmap(m_searchButtonRectangle.size(), (isEnabled() ? QIcon::Active : QIcon::Disabled)));
	}
}

void SearchWidget::resizeEvent(QResizeEvent *event)
{
	LineEditWidget::resizeEvent(event);

	updateGeometries();
}

void SearchWidget::focusInEvent(QFocusEvent *event)
{
	LineEditWidget::focusInEvent(event);

	activate(event->reason());
}

void SearchWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
	{
		sendRequest(text().trimmed());

		event->accept();

		return;
	}

	if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		if (m_isSearchEngineLocked)
		{
			QWidget::keyPressEvent(event);

			return;
		}

		disconnect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));

		m_isIgnoringActivation = true;
	}

	LineEditWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		m_isIgnoringActivation = false;

		setText(m_query);

		connect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));
	}
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-undo")), tr("Undo"), this, SLOT(undo()), QKeySequence(QKeySequence::Undo))->setEnabled(isUndoAvailable());
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-redo")), tr("Redo"), this, SLOT(redo()), QKeySequence(QKeySequence::Redo))->setEnabled(isRedoAvailable());
	menu.addSeparator();
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-cut")), tr("Cut"), this, SLOT(cut()), QKeySequence(QKeySequence::Cut))->setEnabled(hasSelectedText());
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-copy")), tr("Copy"), this, SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(hasSelectedText());
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-paste")), tr("Paste"), this, SLOT(paste()), QKeySequence(QKeySequence::Paste))->setEnabled(!QApplication::clipboard()->text().isEmpty());
	menu.addAction(tr("Paste and Go"), this, SLOT(pasteAndGo()))->setEnabled(!QApplication::clipboard()->text().isEmpty());
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-delete")), tr("Delete"), this, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(hasSelectedText());
	menu.addSeparator();
	menu.addAction(tr("Copy to Note"), this, SLOT(copyToNote()))->setEnabled(!text().isEmpty());
	menu.addSeparator();
	menu.addAction(tr("Clear All"), this, SLOT(clear()))->setEnabled(!text().isEmpty());
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-select-all")), tr("Select All"), this, SLOT(selectAll()))->setEnabled(!text().isEmpty());

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QVector<QAction*>(), &menu));
	}

	menu.exec(event->globalPos());
}

void SearchWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_dropdownArrowRectangle.united(m_iconRectangle).contains(event->pos()) || m_addButtonRectangle.contains(event->pos()) || m_searchButtonRectangle.contains(event->pos()))
	{
		setCursor(Qt::ArrowCursor);
	}
	else
	{
		setCursor(Qt::IBeamCursor);
	}

	LineEditWidget::mouseMoveEvent(event);
}

void SearchWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (m_addButtonRectangle.contains(event->pos()))
		{
			QMenu menu(this);
			const QVector<WebWidget::LinkUrl> searchEngines(m_window ? m_window->getContentsWidget()->getSearchEngines() : QVector<WebWidget::LinkUrl>());

			for (int i = 0; i < searchEngines.count(); ++i)
			{
				if (!SearchEnginesManager::hasSearchEngine(searchEngines.at(i).url))
				{
					menu.addAction(tr("Add %1").arg(searchEngines.at(i).title.isEmpty() ? tr("(untitled)") : searchEngines.at(i).title))->setData(searchEngines.at(i).url);
				}
			}

			connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(addSearchEngine(QAction*)));

			menu.exec(mapToGlobal(m_addButtonRectangle.bottomLeft()));
		}
		else if (m_searchButtonRectangle.contains(event->pos()))
		{
			sendRequest();
		}
		else if (!m_isSearchEngineLocked && !isPopupVisible() && m_dropdownArrowRectangle.united(m_iconRectangle).contains(event->pos()))
		{
			showCompletion(true);
		}
	}

	LineEditWidget::mouseReleaseEvent(event);
}

void SearchWidget::wheelEvent(QWheelEvent *event)
{
	LineEditWidget::wheelEvent(event);

	if (m_isSearchEngineLocked)
	{
		return;
	}

	disconnect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));

	m_isIgnoringActivation = true;

	int index(getCurrentIndex().row());
	QStandardItemModel *model(SearchEnginesManager::getSearchEnginesModel());

	if (event->delta() > 0)
	{
		for (int i = 0; i < model->rowCount(); ++i)
		{
			const QModelIndex modelIndex(model->index((index > 0) ? (index - 1) : (model->rowCount() - 1), 0));

			if (modelIndex.data(Qt::AccessibleDescriptionRole).toString().isEmpty())
			{
				setSearchEngine(modelIndex);

				break;
			}

			if (index == 0)
			{
				index = model->rowCount();
			}
			else
			{
				--index;
			}
		}
	}
	else
	{
		for (int i = 0; i < model->rowCount(); ++i)
		{
			const QModelIndex modelIndex(model->index((((index + 1) < model->rowCount()) ? (index + 1) : 0), 0));

			if (modelIndex.data(Qt::AccessibleDescriptionRole).toString().isEmpty())
			{
				setSearchEngine(modelIndex);

				break;
			}

			if (index == model->rowCount() - 1)
			{
				index = 0;
			}
			else
			{
				++index;
			}
		}
	}

	m_isIgnoringActivation = false;

	setText(m_query);

	connect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));
}

void SearchWidget::showCompletion(bool showSearchModel)
{
	QStandardItemModel *model(showSearchModel ? SearchEnginesManager::getSearchEnginesModel() : m_suggester->getModel());

	if (model->rowCount() == 0)
	{
		return;
	}

	PopupViewWidget *popupWidget(getPopup());
	popupWidget->setModel(model);
	popupWidget->setItemDelegate(new SearchDelegate(this));

	if (!isPopupVisible())
	{
		connect(popupWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(setSearchEngine(QModelIndex)));

		showPopup();
	}

	popupWidget->setCurrentIndex(getCurrentIndex());
}

void SearchWidget::sendRequest(const QString &query)
{
	if (!query.isEmpty())
	{
		m_query = query;
	}

	if (!m_isIgnoringActivation)
	{
		if (m_query.isEmpty())
		{
			const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(m_storedSearchEngine));

			if (searchEngine.formUrl.isValid())
			{
				emit requestedOpenUrl(searchEngine.formUrl, WindowsManager::calculateOpenHints());
			}
		}
		else
		{
			emit requestedSearch(m_query, m_storedSearchEngine, WindowsManager::calculateOpenHints());
		}
	}
}

void SearchWidget::pasteAndGo()
{
	paste();
	sendRequest();
}

void SearchWidget::addSearchEngine(QAction *action)
{
	if (action)
	{
		SearchEngineFetchJob *job(new SearchEngineFetchJob(action->data().toUrl(), QString(), true, this));

		connect(job, &SearchEngineFetchJob::jobFinished, [&](bool isSuccess)
		{
			if (!isSuccess)
			{
				QMessageBox::warning(this, tr("Error"), tr("Failed to add search engine."), QMessageBox::Close);
			}
		});
	}
}

void SearchWidget::storeCurrentSearchEngine()
{
	hidePopup();

	disconnect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));
}

void SearchWidget::restoreCurrentSearchEngine()
{
	if (!m_storedSearchEngine.isEmpty())
	{
		setSearchEngine(m_storedSearchEngine);

		m_storedSearchEngine = QString();
	}

	updateGeometries();
	setText(m_query);

	connect(this, SIGNAL(textChanged(QString)), this, SLOT(setQuery(QString)));
}

void SearchWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::AddressField_DropActionOption:
			{
				const QString dropAction(value.toString());

				if (dropAction == QLatin1String("pasteAndGo"))
				{
					setDropMode(LineEditWidget::ReplaceAndNotifyDropMode);
				}
				else if (dropAction == QLatin1String("replace"))
				{
					setDropMode(LineEditWidget::ReplaceDropMode);
				}
				else
				{
					setDropMode(LineEditWidget::PasteDropMode);
				}
			}

			break;
		case SettingsManager::AddressField_SelectAllOnFocusOption:
			setSelectAllOnFocus(value.toBool());

			break;
		case SettingsManager::Search_SearchEnginesSuggestionsOption:
			if (value.toBool() && !m_suggester)
			{
				m_suggester = new SearchSuggester(m_storedSearchEngine, this);

				connect(this, SIGNAL(textEdited(QString)), m_suggester, SLOT(setQuery(QString)));
				connect(m_suggester, SIGNAL(suggestionsChanged(QVector<SearchSuggester::SearchSuggestion>)), this, SLOT(showCompletion()));
			}
			else if (!value.toBool() && m_suggester)
			{
				m_suggester->deleteLater();
				m_suggester = nullptr;

				disconnect(m_suggester, SIGNAL(suggestionsChanged(QVector<SearchSuggester::SearchSuggestion>)), this, SLOT(showCompletion()));
			}

			break;
		default:
			break;
	}
}

void SearchWidget::updateGeometries()
{
	QStyleOptionFrame panel;
	panel.initFrom(this);
	panel.rect = rect();
	panel.lineWidth = 1;

	const QVector<WebWidget::LinkUrl> searchEngines(m_window ? m_window->getContentsWidget()->getSearchEngines() : QVector<WebWidget::LinkUrl>());
	QMargins margins(qMax(((height() - 16) / 2), 2), 0, 2, 0);
	const bool isRightToLeft(layoutDirection() == Qt::RightToLeft);

	m_searchButtonRectangle = QRect();
	m_addButtonRectangle = QRect();
	m_dropdownArrowRectangle = QRect();

	if (isRightToLeft)
	{
		m_iconRectangle = QRect((width() - margins.right() - 20), ((height() - 16) / 2), 16, 16);

		margins.setRight(margins.right() + 20);
	}
	else
	{
		m_iconRectangle = QRect(margins.left(), ((height() - 16) / 2), 16, 16);

		margins.setLeft(margins.left() + 20);
	}

	if (!m_isSearchEngineLocked)
	{
		if (isRightToLeft)
		{
			m_dropdownArrowRectangle = QRect((width() - margins.right() - 14), 0, 14, height());

			margins.setRight(margins.right() + 12);
		}
		else
		{
			m_dropdownArrowRectangle = QRect(margins.left(), 0, 14, height());

			margins.setLeft(margins.left() + 12);
		}
	}

	if (m_options.value(QLatin1String("showSearchButton"), true).toBool())
	{
		if (isRightToLeft)
		{
			m_searchButtonRectangle = QRect(margins.left(), ((height() - 16) / 2), 16, 16);

			margins.setLeft(margins.left() + 20);
		}
		else
		{
			m_searchButtonRectangle = QRect((width() - margins.right() - 20), ((height() - 16) / 2), 16, 16);

			margins.setRight(margins.right() + 20);
		}
	}

	if (m_window && !searchEngines.isEmpty())
	{
		bool hasAllSearchEngines(true);

		for (int i = 0; i < searchEngines.count(); ++i)
		{
			if (!SearchEnginesManager::hasSearchEngine(searchEngines.at(i).url))
			{
				hasAllSearchEngines = false;

				break;
			}
		}

		if (!hasAllSearchEngines && rect().marginsRemoved(margins).width() > 50)
		{
			if (isRightToLeft)
			{
				m_addButtonRectangle = QRect(margins.left(), ((height() - 16) / 2), 16, 16);

				margins.setLeft(margins.left() + 20);
			}
			else
			{
				m_addButtonRectangle = QRect((width() - margins.right() - 20), ((height() - 16) / 2), 16, 16);

				margins.setRight(margins.right() + 20);
			}
		}
	}

	setTextMargins(margins);
}

void SearchWidget::setSearchEngine(QModelIndex index)
{
	if (m_suggester && getPopup()->model() == m_suggester->getModel())
	{
		setText(m_suggester->getModel()->itemFromIndex(index)->text());
		sendRequest();
		hidePopup();

		return;
	}

	if (index.data(Qt::AccessibleDescriptionRole).toString().isEmpty())
	{
		m_storedSearchEngine = index.data(SearchEnginesManager::IdentifierRole).toString();

		if (!m_isSearchEngineLocked)
		{
			SessionsManager::markSessionModified();

			emit searchEngineChanged(m_storedSearchEngine);
		}

		const QString title(index.data(SearchEnginesManager::TitleRole).toString());

		setToolTip(tr("Search using %1").arg(title));
		setPlaceholderText(tr("Search using %1").arg(title));
		setText(m_query);

		if (m_suggester)
		{
			m_suggester->setSearchEngine(m_storedSearchEngine);
			m_suggester->setQuery(QString());
		}

		if (!m_query.isEmpty() && sender() != m_window)
		{
			sendRequest();
		}
	}
	else
	{
		const QString query(m_query);

		if (query != getPopup()->getItem(index)->text())
		{
			setText(query);
		}

		if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("configure"))
		{
			PreferencesDialog dialog(QLatin1String("search"), this);
			dialog.exec();
		}
	}

	setEnabled(true);
	hidePopup();
}

void SearchWidget::setSearchEngine(const QString &searchEngine)
{
	if (m_isSearchEngineLocked && searchEngine != m_options.value(QLatin1String("searchEngine")).toString())
	{
		return;
	}

	const QStringList searchEngines(SearchEnginesManager::getSearchEngines());

	if (searchEngines.isEmpty())
	{
		hidePopup();
		setEnabled(false);
		setToolTip(QString());
		setPlaceholderText(QString());

		return;
	}

	setSearchEngine(getCurrentIndex());

	if (m_suggester)
	{
		m_suggester->setSearchEngine(m_storedSearchEngine);
	}
}

void SearchWidget::setOptions(const QVariantMap &options)
{
	m_options = options;

	if (m_options.contains(QLatin1String("searchEngine")))
	{
		m_isSearchEngineLocked = true;

		setSearchEngine(m_options[QLatin1String("searchEngine")].toString());
	}
	else
	{
		m_isSearchEngineLocked = false;
	}

	resize(size());
}

void SearchWidget::setQuery(const QString &query)
{
	m_query = query;

	if (getPopup()->model() == SearchEnginesManager::getSearchEnginesModel() || m_query.isEmpty())
	{
		hidePopup();
	}
}

void SearchWidget::setWindow(Window *window)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (m_window && !m_window->isAboutToClose() && (!sender() || sender() != m_window))
	{
		m_window->detachSearchWidget(this);

		disconnect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), m_window.data(), SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(searchEngineChanged(QString)), m_window.data(), SLOT(setSearchEngine(QString)));
		disconnect(m_window.data(), SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
		disconnect(m_window.data(), SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(updateGeometries()));
		disconnect(m_window.data(), SIGNAL(searchEngineChanged(QString)), this, SLOT(setSearchEngine(QString)));

		setSearchEngine();
	}

	m_window = window;

	if (window)
	{
		if (mainWindow)
		{
			disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
		}

		window->attachSearchWidget(this);

		setSearchEngine(window->getSearchEngine());

		connect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), window, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));
		connect(this, SIGNAL(searchEngineChanged(QString)), window, SLOT(setSearchEngine(QString)));
		connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
		connect(window, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(updateGeometries()));
		connect(window, SIGNAL(searchEngineChanged(QString)), this, SLOT(setSearchEngine(QString)));

		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

		if (!toolBar || toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(window, SIGNAL(aboutToClose()), this, SLOT(setWindow()));
		}
	}
	else
	{
		if (mainWindow && !mainWindow->isAboutToClose())
		{
			connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
		}

		setSearchEngine();
	}

	updateGeometries();
}

QModelIndex SearchWidget::getCurrentIndex() const
{
	return SearchEnginesManager::getSearchEnginesModel()->index(qMax(0, SearchEnginesManager::getSearchEngines().indexOf(m_storedSearchEngine)), 0);
}

QVariantMap SearchWidget::getOptions() const
{
	return m_options;
}

bool SearchWidget::event(QEvent *event)
{
	if (isEnabled() && event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			if (m_iconRectangle.contains(helpEvent->pos()) || m_dropdownArrowRectangle.contains(helpEvent->pos()))
			{
				QToolTip::showText(helpEvent->globalPos(), tr("Select Search Engine"));

				return true;
			}

			if (m_addButtonRectangle.contains(helpEvent->pos()))
			{
				QToolTip::showText(helpEvent->globalPos(), tr("Add Search Engine…"));

				return true;
			}

			if (m_searchButtonRectangle.contains(helpEvent->pos()))
			{
				QToolTip::showText(helpEvent->globalPos(), tr("Search"));

				return true;
			}
		}
	}

	return LineEditWidget::event(event);
}

}
