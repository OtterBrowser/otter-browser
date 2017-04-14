/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AddressWidget.h"
#include "../../../core/AddressCompletionModel.h"
#include "../../../core/Application.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/InputInterpreter.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ItemViewWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtCore/QMetaEnum>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionFrame>
#include <QtWidgets/QToolTip>

namespace Otter
{

int AddressWidget::m_entryIdentifierEnumerator(-1);

AddressDelegate::AddressDelegate(ViewMode mode, QObject *parent) : QItemDelegate(parent),
	m_displayMode((SettingsManager::getOption(SettingsManager::AddressField_CompletionDisplayModeOption).toString() == QLatin1String("columns")) ? ColumnsMode : CompactMode),
	m_viewMode(mode)
{
	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
}

void AddressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	drawBackground(painter, option, index);

	QRect titleRectangle(option.rect);

	if (static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::HeaderType)
	{
		titleRectangle = titleRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

		if (index.row() != 0)
		{
			QPen pen(Qt::lightGray);
			pen.setWidth(1);
			pen.setStyle(Qt::SolidLine);

			painter->setPen(pen);
			painter->drawLine((option.rect.left() + 5), (option.rect.top() + 3), (option.rect.right() - 5), (option.rect.top() + 3));
		}

		drawDisplay(painter, option, titleRectangle, index.data(AddressCompletionModel::TitleRole).toString());

		return;
	}

	if (option.direction == Qt::RightToLeft)
	{
		titleRectangle.setRight(option.rect.width() - 33);
	}
	else
	{
		titleRectangle.setLeft(33);
	}

	QRect decorationRectangle(option.rect);

	if (option.direction == Qt::RightToLeft)
	{
		decorationRectangle.setLeft(option.rect.width() - 33);
	}
	else
	{
		decorationRectangle.setRight(33);
	}

	decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	QIcon icon(index.data(Qt::DecorationRole).value<QIcon>());

	if (icon.isNull())
	{
		icon = ThemesManager::getIcon(QLatin1String("tab"));
	}

	icon.paint(painter, decorationRectangle, option.decorationAlignment);

	QString url(index.data(Qt::DisplayRole).toString());
	const QString description((m_viewMode == HistoryMode) ? Utils::formatDateTime(index.data(AddressCompletionModel::TimeVisitedRole).toDateTime()) : index.data(AddressCompletionModel::TitleRole).toString());
	QStyleOptionViewItem linkOption(option);

	if (static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) != AddressCompletionModel::SearchSuggestionType)
	{
		linkOption.palette.setColor(QPalette::Text, option.palette.color(QPalette::Link));
	}

	if (m_displayMode == ColumnsMode)
	{
		const int maxUrlWidth(option.rect.width() / 2);

		url = option.fontMetrics.elidedText(url, Qt::ElideRight, (maxUrlWidth - 40));

		drawDisplay(painter, linkOption, titleRectangle, url);

		if (!description.isEmpty())
		{
			if (option.direction == Qt::RightToLeft)
			{
				titleRectangle.setRight(maxUrlWidth);
			}
			else
			{
				titleRectangle.setLeft(maxUrlWidth);
			}

			drawDisplay(painter, option, titleRectangle, description);
		}

		return;
	}

	drawDisplay(painter, linkOption, titleRectangle, url);

	if (!description.isEmpty())
	{
		const int urlLength(option.fontMetrics.width(url + QLatin1Char(' ')));

		if (urlLength < titleRectangle.width())
		{
			if (option.direction == Qt::RightToLeft)
			{
				titleRectangle.setRight(option.rect.width() - (urlLength + 33));
			}
			else
			{
				titleRectangle.setLeft(urlLength + 33);
			}

			drawDisplay(painter, option, titleRectangle, QLatin1String("- ") + description);
		}
	}
}

void AddressDelegate::handleOptionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::AddressField_CompletionDisplayModeOption)
	{
		m_displayMode = ((value.toString() == QLatin1String("columns")) ? ColumnsMode : CompactMode);
	}
}

QSize AddressDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.row() != 0 && static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::HeaderType)
	{
		size.setHeight(option.fontMetrics.height() * 1.75);
	}
	else
	{
		size.setHeight(option.fontMetrics.height() * 1.25);
	}

	return size;
}

AddressWidget::AddressWidget(Window *window, QWidget *parent) : LineEditWidget(parent),
	m_window(nullptr),
	m_completionModel(new AddressCompletionModel(this)),
	m_clickedEntry(UnknownEntry),
	m_hoveredEntry(UnknownEntry),
	m_completionModes(NoCompletionMode),
	m_hints(SessionsManager::DefaultOpen),
	m_isNavigatingCompletion(false),
	m_isUsingSimpleMode(false)
{
	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (!toolBar)
	{
		m_isUsingSimpleMode = true;
	}

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumWidth(100);
	setWindow(window);
	handleOptionChanged(SettingsManager::AddressField_CompletionModeOption, SettingsManager::getOption(SettingsManager::AddressField_CompletionModeOption));
	handleOptionChanged(SettingsManager::AddressField_DropActionOption, SettingsManager::getOption(SettingsManager::AddressField_DropActionOption));
	handleOptionChanged(SettingsManager::AddressField_LayoutOption, SettingsManager::getOption(SettingsManager::AddressField_LayoutOption));
	handleOptionChanged(SettingsManager::AddressField_SelectAllOnFocusOption, SettingsManager::getOption(SettingsManager::AddressField_SelectAllOnFocusOption));

	if (toolBar)
	{
		setPlaceholderText(tr("Enter address or search…"));

		connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));

		if (toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
		}
	}

	connect(this, SIGNAL(textDropped(QString)), this, SLOT(handleUserInput(QString)));
	connect(m_completionModel, SIGNAL(completionReady(QString)), this, SLOT(setCompletion(QString)));
	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateGeometries()));
}

void AddressWidget::changeEvent(QEvent *event)
{
	LineEditWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			if (!m_isUsingSimpleMode)
			{
				setPlaceholderText(tr("Enter address or search…"));
			}

			break;
		case QEvent::LayoutDirectionChange:
			updateGeometries();

			break;
		default:
			break;
	}
}

void AddressWidget::paintEvent(QPaintEvent *event)
{
	LineEditWidget::paintEvent(event);

	QPainter painter(this);

	if (m_entries.contains(HistoryDropdownEntry))
	{
		QStyleOption arrow;
		arrow.initFrom(this);
		arrow.rect = m_entries[HistoryDropdownEntry].rectangle;

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, this);
	}

	if (m_isUsingSimpleMode)
	{
		return;
	}

	QHash<EntryIdentifier, EntryDefinition>::const_iterator iterator;

	for (iterator = m_entries.begin(); iterator != m_entries.end(); ++iterator)
	{
		if (!iterator.value().icon.isNull())
		{
			iterator.value().icon.paint(&painter, iterator.value().rectangle, Qt::AlignCenter, iterator.value().mode);
		}
	}
}

void AddressWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		event->accept();
	}

	LineEditWidget::dragEnterEvent(event);
}

void AddressWidget::resizeEvent(QResizeEvent *event)
{
	LineEditWidget::resizeEvent(event);

	updateGeometries();
}

void AddressWidget::focusInEvent(QFocusEvent *event)
{
	if (event->reason() == Qt::MouseFocusReason)
	{
		const EntryIdentifier entry(getEntry(mapFromGlobal(QCursor::pos())));

		if (entry != UnknownEntry && entry != AddressEntry && entry != HistoryDropdownEntry)
		{
			Application::triggerAction(ActionsManager::ActivateContentAction, QVariantMap(), this);

			return;
		}
	}

	LineEditWidget::focusInEvent(event);

	activate(event->reason());
}

void AddressWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Down && !isPopupVisible())
	{
		m_completionModel->setFilter(QString(), AddressCompletionModel::TypedHistoryCompletionType);

		showCompletion(true);
	}
	else if (m_window && event->key() == Qt::Key_Escape)
	{
		const QUrl url(m_window->getUrl());
		const QString text(this->text().trimmed());

		if (text.isEmpty() || text != url.toString())
		{
			setText(Utils::isUrlEmpty(url) ? QString() : url.toString());

			if (!text.isEmpty() && SettingsManager::getOption(SettingsManager::AddressField_SelectAllOnFocusOption).toBool())
			{
				selectAll();
			}
		}
		else
		{
			m_window->setFocus();
		}
	}
	else if (!m_isUsingSimpleMode && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
	{
		handleUserInput(text().trimmed(), SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen, Qt::LeftButton, event->modifiers()));
	}

	LineEditWidget::keyPressEvent(event);
}

void AddressWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const EntryIdentifier entry(getEntry(event->pos()));
	QMenu menu(this);

	if (entry == UnknownEntry || entry == AddressEntry)
	{
		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-undo")), tr("Undo"), this, SLOT(undo()), QKeySequence(QKeySequence::Undo))->setEnabled(isUndoAvailable());
		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-redo")), tr("Redo"), this, SLOT(redo()), QKeySequence(QKeySequence::Redo))->setEnabled(isRedoAvailable());
		menu.addSeparator();
		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-cut")), tr("Cut"), this, SLOT(cut()), QKeySequence(QKeySequence::Cut))->setEnabled(hasSelectedText());
		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-copy")), tr("Copy"), this, SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(hasSelectedText());
		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-paste")), tr("Paste"), this, SLOT(paste()), QKeySequence(QKeySequence::Paste))->setEnabled(!QApplication::clipboard()->text().isEmpty());

		if (!m_isUsingSimpleMode)
		{
			menu.addAction(ActionsManager::getAction(ActionsManager::PasteAndGoAction, this));
		}

		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-delete")), tr("Delete"), this, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(hasSelectedText());
		menu.addSeparator();
		menu.addAction(tr("Copy to Note"), this, SLOT(copyToNote()))->setEnabled(!text().isEmpty());
		menu.addSeparator();
		menu.addAction(tr("Clear All"), this, SLOT(clear()))->setEnabled(!text().isEmpty());
		menu.addAction(ThemesManager::getIcon(QLatin1String("edit-select-all")), tr("Select All"), this, SLOT(selectAll()), QKeySequence(QKeySequence::SelectAll))->setEnabled(!text().isEmpty());
	}
	else
	{
		const QUrl url(getUrl());

		if (entry == WebsiteInformationEntry && !Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
		{
			if (m_window)
			{
				menu.addAction(m_window->getContentsWidget()->getAction(ActionsManager::WebsiteInformationAction));
			}
			else
			{
				Action *websiteInformationAction(new Action(ActionsManager::WebsiteInformationAction));
				websiteInformationAction->setEnabled(false);

				menu.addAction(websiteInformationAction);
			}

			menu.addSeparator();
		}

		menu.addAction(tr("Remove this Icon"), this, SLOT(removeEntry()))->setData(entry);
	}

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QVector<QAction*>(), &menu));
	}

	menu.exec(event->globalPos());
}

void AddressWidget::mousePressEvent(QMouseEvent *event)
{
	m_clickedEntry = ((event->button() == Qt::LeftButton) ? getEntry(event->pos()) : UnknownEntry);

	if (m_clickedEntry == WebsiteInformationEntry || m_clickedEntry == FaviconEntry)
	{
		m_dragStartPosition = event->pos();
	}
	else
	{
		m_dragStartPosition = QPoint();
	}

	LineEditWidget::mousePressEvent(event);
}

void AddressWidget::mouseMoveEvent(QMouseEvent *event)
{
	const EntryIdentifier entry(getEntry(event->pos()));

	if (entry != m_hoveredEntry)
	{
		if (entry == UnknownEntry || entry == AddressEntry)
		{
			setCursor(Qt::IBeamCursor);
		}
		else
		{
			setCursor(Qt::ArrowCursor);
		}

		m_hoveredEntry = entry;
	}

	if (!startDrag(event))
	{
		LineEditWidget::mouseMoveEvent(event);
	}
}

void AddressWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && m_clickedEntry == getEntry(event->pos()))
	{
		switch (m_clickedEntry)
		{
			case WebsiteInformationEntry:
				m_window->triggerAction(ActionsManager::WebsiteInformationAction);

				event->accept();

				return;
			case ListFeedsEntry:
				{
					const QVector<WebWidget::LinkUrl> feeds((m_window && m_window->getLoadingState() == WebWidget::FinishedLoadingState) ? m_window->getContentsWidget()->getFeeds() : QVector<WebWidget::LinkUrl>());

					if (feeds.count() == 1 && m_window)
					{
						m_window->setUrl(feeds.at(0).url);
					}
					else if (feeds.count() > 1)
					{
						QMenu menu;

						for (int i = 0; i < feeds.count(); ++i)
						{
							menu.addAction(feeds.at(i).title.isEmpty() ? tr("(Untitled)") : feeds.at(i).title)->setData(feeds.at(i).url);
						}

						connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(openFeed(QAction*)));

						menu.exec(mapToGlobal(m_entries.value(ListFeedsEntry).rectangle.bottomLeft()));
					}

					event->accept();
				}

				return;
			case BookmarkEntry:
				{
					const QUrl url(getUrl());

					if (!Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
					{
						if (BookmarksManager::hasBookmark(url))
						{
							BookmarksManager::removeBookmark(url);
						}
						else
						{
							QMenu menu;
							menu.addAction(tr("Add to Bookmarks"));
							menu.addAction(tr("Add to Start Page"))->setData(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption));

							connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(addBookmark(QAction*)));

							menu.exec(mapToGlobal(m_entries.value(BookmarkEntry).rectangle.bottomLeft()));
						}

						updateGeometries();
					}

					event->accept();
				}

				return;
			case LoadPluginsEntry:
				m_window->getContentsWidget()->triggerAction(ActionsManager::LoadPluginsAction);

				event->accept();

				return;
			case FillPasswordEntry:
				m_window->getContentsWidget()->triggerAction(ActionsManager::FillPasswordAction);

				event->accept();

				return;
			case HistoryDropdownEntry:
				if (!isPopupVisible())
				{
					m_completionModel->setFilter(QString(), AddressCompletionModel::TypedHistoryCompletionType);

					showCompletion(true);
				}

				break;
			default:
				break;
		}
	}

	if (event->button() == Qt::MiddleButton && text().isEmpty() && !QApplication::clipboard()->text().isEmpty() && SettingsManager::getOption(SettingsManager::AddressField_PasteAndGoOnMiddleClickOption).toBool())
	{
		handleUserInput(QApplication::clipboard()->text().trimmed(), SessionsManager::CurrentTabOpen);

		event->accept();
	}

	m_clickedEntry = UnknownEntry;

	LineEditWidget::mouseReleaseEvent(event);
}

void AddressWidget::addBookmark(QAction *action)
{
	if (action && m_window)
	{
		const QUrl url(getUrl().adjusted(QUrl::RemovePassword));

		if (action->data().isNull())
		{
			BookmarkPropertiesDialog dialog(url, m_window->getTitle(), QString(), nullptr, -1, true, this);
			dialog.exec();
		}
		else
		{
			BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, url, m_window->getTitle(), BookmarksManager::getModel()->getItem(action->data().toString()));
		}
	}
}

void AddressWidget::openFeed(QAction *action)
{
	if (action && m_window)
	{
		m_window->setUrl(action->data().toUrl());
	}
}

void AddressWidget::openUrl(const QString &url)
{
	setUrl(url);
	handleUserInput(url, SessionsManager::CurrentTabOpen);
}

void AddressWidget::openUrl(const QModelIndex &index)
{
	hidePopup();

	if (!index.isValid())
	{
		return;
	}

	if (static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::SearchSuggestionType)
	{
		emit requestedSearch(index.data(AddressCompletionModel::TextRole).toString(), QString(), SessionsManager::CurrentTabOpen);
	}
	else
	{
		const QString url(index.data(AddressCompletionModel::UrlRole).toUrl().toString());

		setUrl(url);
		handleUserInput(url, SessionsManager::CurrentTabOpen);
	}
}

void AddressWidget::removeEntry()
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action)
	{
		QStringList layout(SettingsManager::getOption(SettingsManager::AddressField_LayoutOption).toStringList());
		QString name(metaObject()->enumerator(m_entryIdentifierEnumerator).valueToKey(action->data().toInt()));

		if (!name.isEmpty())
		{
			name.chop(5);
			name[0] = name.at(0).toLower();

			layout.removeAll(name);

			SettingsManager::setValue(SettingsManager::AddressField_LayoutOption, layout);
		}
	}
}

void AddressWidget::showCompletion(bool isTypedHistory)
{
	PopupViewWidget *popupWidget(getPopup());
	popupWidget->setModel(m_completionModel);
	popupWidget->setItemDelegate(new AddressDelegate(isTypedHistory ? AddressDelegate::HistoryMode : AddressDelegate::CompletionMode, popupWidget));

	if (!isPopupVisible())
	{
		connect(popupWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(openUrl(QModelIndex)));
		connect(popupWidget->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(setTextFromIndex(QModelIndex)));

		showPopup();
	}

	popupWidget->setCurrentIndex(m_completionModel->index(0, 0));
}

void AddressWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::AddressField_CompletionModeOption:
			{
				const QString completionMode(value.toString());

				if (completionMode == QLatin1String("inlineAndPopup"))
				{
					m_completionModes = (InlineCompletionMode | PopupCompletionMode);
				}
				else if (completionMode == QLatin1String("inline"))
				{
					m_completionModes = InlineCompletionMode;
				}
				else if (completionMode == QLatin1String("popup"))
				{
					m_completionModes = PopupCompletionMode;
				}

				disconnect(this, SIGNAL(textEdited(QString)), m_completionModel, SLOT(setFilter(QString)));

				if (m_completionModes != NoCompletionMode)
				{
					connect(this, SIGNAL(textEdited(QString)), m_completionModel, SLOT(setFilter(QString)));
				}
			}

			break;
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
		case SettingsManager::AddressField_LayoutOption:
			if (m_isUsingSimpleMode)
			{
				m_layout = {AddressEntry, HistoryDropdownEntry};
			}
			else
			{
				if (m_entryIdentifierEnumerator < 0)
				{
					m_entryIdentifierEnumerator = metaObject()->indexOfEnumerator("EntryIdentifier");
				}

				const QStringList rawLayout(value.toStringList());
				QVector<EntryIdentifier> layout;
				layout.reserve(rawLayout.count());

				for (int i = 0; i < rawLayout.count(); ++i)
				{
					QString name(rawLayout.at(i) + QLatin1String("Entry"));
					name[0] = name.at(0).toUpper();

					const EntryIdentifier identifier(static_cast<EntryIdentifier>(metaObject()->enumerator(m_entryIdentifierEnumerator).keyToValue(name.toLatin1())));

					if (identifier > UnknownEntry && !layout.contains(identifier))
					{
						layout.append(identifier);
					}
				}

				if (!layout.contains(AddressEntry))
				{
					layout.prepend(AddressEntry);
				}

				m_layout = layout;
			}

			updateGeometries();

			break;
		default:
			break;
	}
}

void AddressWidget::handleUserInput(const QString &text, SessionsManager::OpenHints hints)
{
	if (hints == SessionsManager::DefaultOpen)
	{
		hints = SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen);
	}

	if (!text.isEmpty())
	{
		InputInterpreter *interpreter(new InputInterpreter(this));

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), this, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)));

		interpreter->interpret(text, hints);
	}
}

void AddressWidget::updateGeometries()
{
	QHash<EntryIdentifier, EntryDefinition> entries;
	QVector<EntryDefinition> leadingEntries;
	QVector<EntryDefinition> trailingEntries;
	const int offset(qMax(((height() - 16) / 2), 2));
	QMargins margins(offset, 0, offset, 0);
	int availableWidth(width() - margins.left() - margins.right());
	bool isLeading(true);
	bool isRightToLeft(layoutDirection() == Qt::RightToLeft);

	if (m_layout.contains(WebsiteInformationEntry))
	{
		availableWidth -= 20;
	}

	if (m_layout.contains(HistoryDropdownEntry))
	{
		availableWidth -= 16;
	}

	if (isRightToLeft)
	{
		isLeading = false;
	}

	for (int i = 0; i < m_layout.count(); ++i)
	{
		EntryDefinition definition;
		definition.identifier = m_layout.at(i);

		switch (m_layout.at(i))
		{
			case AddressEntry:
				isLeading = !isLeading;

				break;
			case WebsiteInformationEntry:
				{
					const QUrl url(getUrl());
					QString icon(QLatin1String("unknown"));
					const WebWidget::ContentStates state(m_window ? m_window->getContentState() : WebWidget::UnknownContentState);

					if (state.testFlag(WebWidget::FraudContentState))
					{
						icon = QLatin1String("badge-fraud");
					}
					else if (state.testFlag(WebWidget::MixedContentState))
					{
						icon = QLatin1String("badge-mixed");
					}
					else if (state.testFlag(WebWidget::SecureContentState))
					{
						icon = QLatin1String("badge-secure");
					}
					else if (state.testFlag(WebWidget::RemoteContentState))
					{
						icon = QLatin1String("badge-remote");
					}
					else if (state.testFlag(WebWidget::LocalContentState))
					{
						icon = QLatin1String("badge-local");
					}
					else if (state.testFlag(WebWidget::ApplicationContentState))
					{
						icon = QLatin1String("otter-browser");
					}

					if (!Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
					{
						definition.title = QT_TR_NOOP("Show website information");
					}

					definition.icon = ThemesManager::getIcon(icon, false);
				}

				break;
			case FaviconEntry:
				definition.icon = (m_window ? m_window->getIcon() : ThemesManager::getIcon((SessionsManager::isPrivate() ? QLatin1String("tab-private") : QLatin1String("tab")), false));

				break;
			case ListFeedsEntry:
				if (!m_window || m_window->isAboutToClose() || m_window->getLoadingState() != WebWidget::FinishedLoadingState || m_window->getContentsWidget()->getFeeds().isEmpty())
				{
					continue;
				}

				definition.title = QT_TR_NOOP("Show feed list");
				definition.icon = ThemesManager::getIcon(QLatin1String("application-rss+xml"), false);

				break;
			case BookmarkEntry:
				{
					const QUrl url(getUrl());

					definition.icon = ThemesManager::getIcon(QLatin1String("bookmarks"), false);

					if (BookmarksManager::hasBookmark(url))
					{
						definition.title = QT_TR_NOOP("Remove bookmark");
						definition.mode = QIcon::Normal;
					}
					else
					{
						if (Utils::isUrlEmpty(url) || url.scheme() == QLatin1String("about"))
						{
							definition.title = QString();
						}
						else
						{
							definition.title = QT_TR_NOOP("Add bookmark");
						}

						definition.mode = QIcon::Disabled;
					}
				}

				break;
			case LoadPluginsEntry:
				{
					if (!m_window || m_window->isAboutToClose() || m_window->getLoadingState() != WebWidget::FinishedLoadingState)
					{
						continue;
					}

					Action *loadPluginsAction(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction));

					if (!loadPluginsAction || !loadPluginsAction->isEnabled())
					{
						continue;
					}

					definition.title = QT_TR_NOOP("Load all plugins on the page");
					definition.icon = ThemesManager::getIcon(QLatin1String("preferences-plugin"), false);
				}

				break;
			case FillPasswordEntry:
				{
					const QUrl url(getUrl());

					if (!m_window || m_window->isAboutToClose() || m_window->getLoadingState() != WebWidget::FinishedLoadingState || Utils::isUrlEmpty(url) || url.scheme() == QLatin1String("about") || !PasswordsManager::hasPasswords(url, PasswordsManager::FormPassword))
					{
						continue;
					}

					definition.title = QT_TR_NOOP("Log in");
					definition.icon = ThemesManager::getIcon(QLatin1String("fill-password"), false);
				}

				break;
			default:
				break;
		}

		switch (m_layout.at(i))
		{
			case AddressEntry:
			case HistoryDropdownEntry:
			case WebsiteInformationEntry:
				break;
			default:
				availableWidth -= 20;

				if (availableWidth < 100)
				{
					continue;
				}

				break;
		}

		if (isLeading)
		{
			if (isRightToLeft)
			{
				leadingEntries.prepend(definition);
			}
			else
			{
				leadingEntries.append(definition);
			}
		}
		else
		{
			if (isRightToLeft)
			{
				trailingEntries.append(definition);
			}
			else
			{
				trailingEntries.prepend(definition);
			}
		}
	}

	for (int i = 0; i < leadingEntries.count(); ++i)
	{
		switch (leadingEntries.at(i).identifier)
		{
			case WebsiteInformationEntry:
			case FaviconEntry:
			case ListFeedsEntry:
			case BookmarkEntry:
			case LoadPluginsEntry:
			case FillPasswordEntry:
				leadingEntries[i].rectangle = QRect(margins.left(), ((height() - 16) / 2), 16, 16);

				margins.setLeft(margins.left() + 20);

				break;
			case HistoryDropdownEntry:
				leadingEntries[i].rectangle = QRect(margins.left(), 0, 14, height());

				margins.setLeft(margins.left() + 16);

				break;
			default:
				break;
		}

		entries[leadingEntries.at(i).identifier] = leadingEntries.at(i);
	}

	for (int i = 0; i < trailingEntries.count(); ++i)
	{
		switch (trailingEntries.at(i).identifier)
		{
			case WebsiteInformationEntry:
			case FaviconEntry:
			case ListFeedsEntry:
			case BookmarkEntry:
			case LoadPluginsEntry:
			case FillPasswordEntry:
				trailingEntries[i].rectangle = QRect((width() - margins.right() - 20), ((height() - 16) / 2), 16, 16);

				margins.setRight(margins.right() + 20);

				break;
			case HistoryDropdownEntry:
				trailingEntries[i].rectangle = QRect((width() - margins.right() - 14), 0, 14, height());

				margins.setRight(margins.right() + 16);

				break;
			default:
				break;
		}

		entries[trailingEntries.at(i).identifier] = trailingEntries.at(i);
	}

	m_entries = entries;

	if (margins.left() > offset)
	{
		margins.setLeft(margins.left() - 2);
	}

	if (margins.right() > offset)
	{
		margins.setRight(margins.right() + 2);
	}

	setTextMargins(margins);
}

void AddressWidget::setCompletion(const QString &filter)
{
	if (filter.isEmpty() || m_completionModel->rowCount() == 0)
	{
		hidePopup();

		LineEditWidget::setCompletion(QString());

		return;
	}

	if (m_completionModes.testFlag(PopupCompletionMode))
	{
		showCompletion(false);
	}

	if (m_completionModes.testFlag(InlineCompletionMode))
	{
		QString matchedText;

		for (int i = 0; i < m_completionModel->rowCount(); ++i)
		{
			matchedText = m_completionModel->index(i).data(AddressCompletionModel::MatchRole).toString();

			if (!matchedText.isEmpty())
			{
				LineEditWidget::setCompletion(matchedText);

				break;
			}
		}
	}
}

void AddressWidget::setWindow(Window *window)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (m_window && !m_window->isAboutToClose() && (!sender() || sender() != m_window))
	{
		m_window->detachAddressWidget(this);

		disconnect(this, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,SessionsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), m_window.data(), SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), m_window.data(), SLOT(handleSearchRequest(QString,QString,SessionsManager::OpenHints)));
		disconnect(m_window.data(), SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
		disconnect(m_window.data(), SIGNAL(urlChanged(QUrl,bool)), this, SLOT(setUrl(QUrl,bool)));
		disconnect(m_window.data(), SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		disconnect(m_window.data(), SIGNAL(contentStateChanged(WebWidget::ContentStates)), this, SLOT(updateGeometries()));
		disconnect(m_window.data(), SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SLOT(updateGeometries()));
	}

	m_window = window;

	if (window)
	{
		if (mainWindow)
		{
			disconnect(this, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), mainWindow, SLOT(open(QUrl,SessionsManager::OpenHints)));
			disconnect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), mainWindow, SLOT(open(BookmarksItem*,SessionsManager::OpenHints)));
			disconnect(this, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), mainWindow, SLOT(search(QString,QString,SessionsManager::OpenHints)));
		}

		window->attachAddressWidget(this);

		connect(this, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), window, SLOT(handleOpenUrlRequest(QUrl,SessionsManager::OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), window, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), window, SLOT(handleSearchRequest(QString,QString,SessionsManager::OpenHints)));
		connect(window, SIGNAL(urlChanged(QUrl,bool)), this, SLOT(setUrl(QUrl,bool)));
		connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		connect(window, SIGNAL(contentStateChanged(WebWidget::ContentStates)), this, SLOT(updateGeometries()));
		connect(window, SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SLOT(updateGeometries()));

		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

		if (!toolBar || toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(window, SIGNAL(aboutToClose()), this, SLOT(setWindow()));
		}
	}
	else if (mainWindow && !mainWindow->isAboutToClose() && !m_isUsingSimpleMode)
	{
		connect(this, SIGNAL(requestedOpenUrl(QUrl,SessionsManager::OpenHints)), mainWindow, SLOT(open(QUrl,SessionsManager::OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,SessionsManager::OpenHints)), mainWindow, SLOT(open(BookmarksItem*,SessionsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,SessionsManager::OpenHints)), mainWindow, SLOT(search(QString,QString,SessionsManager::OpenHints)));
	}

	setIcon(window ? window->getIcon() : QIcon());
	setUrl(window ? window->getUrl() : QUrl());
	update();
}

void AddressWidget::setTextFromIndex(const QModelIndex &index)
{
	if (m_isNavigatingCompletion)
	{
		m_isNavigatingCompletion = false;

		setText(index.data(AddressCompletionModel::TextRole).toString());
	}
}

void AddressWidget::setUrl(const QUrl &url, bool force)
{
	if (!m_isUsingSimpleMode)
	{
		updateGeometries();
	}

	if (!m_window || ((force || !hasFocus()) && url.scheme() != QLatin1String("javascript")))
	{
		const QString text(Utils::isUrlEmpty(url) ? QString() : url.toString());

		setToolTip(text);
		setText(text);
		setCursorPosition(0);
	}
}

void AddressWidget::setIcon(const QIcon &icon)
{
	if (m_layout.contains(FaviconEntry))
	{
		m_entries[FaviconEntry].icon = (icon.isNull() ? ThemesManager::getIcon((SessionsManager::isPrivate() ? QLatin1String("tab-private") : QLatin1String("tab")), false) : icon);

		update();
	}
}

QUrl AddressWidget::getUrl() const
{
	return (m_window ? m_window->getUrl() : QUrl(QLatin1String("about:blank")));
}

AddressWidget::EntryIdentifier AddressWidget::getEntry(const QPoint &position) const
{
	QHash<EntryIdentifier, EntryDefinition>::const_iterator iterator;

	for (iterator = m_entries.begin(); iterator != m_entries.end(); ++iterator)
	{
		if (iterator.value().rectangle.contains(position))
		{
			return iterator.key();
		}
	}

	return UnknownEntry;
}

bool AddressWidget::startDrag(QMouseEvent *event)
{
	const QUrl url(getUrl());

	if (event->buttons().testFlag(Qt::LeftButton) && !m_dragStartPosition.isNull() && (event->pos() - m_dragStartPosition).manhattanLength() >= QApplication::startDragDistance() && url.isValid())
	{
		Utils::startLinkDrag(url, (m_window ? m_window->getTitle() : QString()), ((m_window ? m_window->getIcon() : ThemesManager::getIcon(QLatin1String("tab"))).pixmap(16, 16)), this);

		return true;
	}

	return false;
}

bool AddressWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			const EntryIdentifier entry(getEntry(helpEvent->pos()));

			if (entry != UnknownEntry && entry != AddressEntry)
			{
				const QString title(m_entries[entry].title);

				if (!title.isEmpty())
				{
					QToolTip::showText(helpEvent->globalPos(), tr(title.toUtf8().constData()));

					return true;
				}
			}
		}
	}

	return LineEditWidget::event(event);
}

}
