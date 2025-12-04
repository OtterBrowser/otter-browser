/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "AddressCompletionModel.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/Application.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/FeedsManager.h"
#include "../../../core/InputInterpreter.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

int AddressWidget::m_entryIdentifierEnumerator(-1);

AddressDelegate::AddressDelegate(const QString &highlight, ViewMode mode, QObject *parent) : QStyledItemDelegate(parent),
	m_highlight(highlight),
	m_displayMode((SettingsManager::getOption(SettingsManager::AddressField_CompletionDisplayModeOption).toString() == QLatin1String("columns")) ? ColumnsMode : CompactMode),
	m_viewMode(mode)
{
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, [&](int identifier, const QVariant &value)
	{
		if (identifier == SettingsManager::AddressField_CompletionDisplayModeOption)
		{
			m_displayMode = ((value.toString() == QLatin1String("columns")) ? ColumnsMode : CompactMode);
		}
	});
}

void AddressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QRect textRectangle(option.rect);
	const bool isRightToLeft(option.direction == Qt::RightToLeft);

	if (static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::CompletionEntry::HeaderType)
	{
		QStyleOptionViewItem headerOption(option);
		headerOption.rect = textRectangle.marginsRemoved(QMargins(0, 2, (isRightToLeft ? 3 : 0), 2));
		headerOption.text = index.data(AddressCompletionModel::TitleRole).toString();

		if (index.row() > 0)
		{
			QPen pen(option.palette.color(QPalette::Disabled, QPalette::Text).lighter());
			pen.setWidth(1);
			pen.setStyle(Qt::SolidLine);

			painter->save();
			painter->setPen(pen);
			painter->drawLine((option.rect.left() + 5), (option.rect.top() + 3), (option.rect.right() - 5), (option.rect.top() + 3));
			painter->restore();
		}

		QStyledItemDelegate::paint(painter, headerOption, index);

		return;
	}

	const QString url(index.data(Qt::DisplayRole).toString());
	const QString description((m_viewMode == HistoryMode) ? Utils::formatDateTime(index.data(AddressCompletionModel::TimeVisitedRole).toDateTime()) : index.data(AddressCompletionModel::TitleRole).toString());
	const bool isSearchSuggestion(static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::CompletionEntry::SearchSuggestionType);
	const bool isSelected(option.state.testFlag(QStyle::State_Selected));

	if (isSelected)
	{
		painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));

		if (index.data(AddressCompletionModel::IsRemovableRole).toBool())
		{
			if (isRightToLeft)
			{
				textRectangle.setLeft(textRectangle.left() + option.rect.height());
			}
			else
			{
				textRectangle.setRight(textRectangle.right() - option.rect.height());
			}
		}
	}

	QRect decorationRectangle(option.rect);

	if (isRightToLeft)
	{
		decorationRectangle.setLeft(option.rect.width() - option.rect.height() - 5);

		textRectangle.setRight(option.rect.width() - option.rect.height() - 10);
	}
	else
	{
		decorationRectangle.setRight(option.rect.height());

		textRectangle.setLeft(option.rect.height());
	}

	decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	QIcon icon(index.data(Qt::DecorationRole).value<QIcon>());

	if (icon.isNull())
	{
		icon = ThemesManager::createIcon(QLatin1String("tab"));
	}

	icon.paint(painter, decorationRectangle, option.decorationAlignment);

	const QString highlight(isSearchSuggestion ? QString() : m_highlight);
	const TextSegment urlSegment(url, option.palette.color(isSelected ? QPalette::HighlightedText : (isSearchSuggestion ? QPalette::Text : QPalette::Link)));
	TextSegment descriptionSegment(description, (option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::Text)));

	textRectangle.adjust(2, 0, 2, 0);

	if (m_displayMode == ColumnsMode)
	{
		const int sectionWidth(textRectangle.width() / 3);
		QRect urlRectangle(textRectangle);
		urlRectangle.setRight(textRectangle.right() - 2 - sectionWidth);

		drawCompletionText(painter, option.font, highlightSegments(highlight, {urlSegment}), urlRectangle, isRightToLeft);

		if (!description.isEmpty())
		{
			QRect descriptionRectangle(textRectangle);
			descriptionRectangle.setLeft(urlRectangle.right() + 4);

			drawCompletionText(painter, option.font, highlightSegments(highlight, {descriptionSegment}), descriptionRectangle, isRightToLeft);
		}
	}
	else
	{
		if (description.isEmpty())
		{
			drawCompletionText(painter, option.font, highlightSegments(highlight, {urlSegment}), textRectangle, isRightToLeft);
		}
		else
		{
			descriptionSegment.text = QLatin1Char(' ') + QChar(8212) + QLatin1Char(' ') + descriptionSegment.text;

			drawCompletionText(painter, option.font, highlightSegments(highlight, {urlSegment, descriptionSegment}), textRectangle, isRightToLeft);
		}
	}
}

void AddressDelegate::drawCompletionText(QPainter *painter, const QFont &font, const QVector<TextSegment> &segments, const QRect &rectangle, bool isRightToLeft) const
{
	QRect availableRectangle(rectangle);
	QFont highlightFont(font);
	highlightFont.setBold(true);

	const QFontMetrics fontMetrics(font);
	const QFontMetrics highlightFontMetrics(highlightFont);
	const int xLength(highlightFontMetrics.horizontalAdvance(QLatin1Char('X')));

	for (int i = 0; i < segments.count(); ++i)
	{
		const TextSegment segment(segments.at(i));
		const QFontMetrics segmentFontMetrics(segment.isHighlighted ? highlightFontMetrics : fontMetrics);
		const int maximumLength(availableRectangle.width() - xLength);
		const int length(segmentFontMetrics.horizontalAdvance(segment.text));

		painter->setFont(segment.isHighlighted ? highlightFont : font);
		painter->setPen(segment.color);

		if (length >= maximumLength)
		{
			painter->drawText(availableRectangle, Qt::AlignVCenter, Utils::elideText(segment.text, segmentFontMetrics, nullptr, maximumLength));

			break;
		}

		painter->drawText(availableRectangle, Qt::AlignVCenter, segment.text);

		if (isRightToLeft)
		{
			availableRectangle.setRight(availableRectangle.right() - length);
		}
		else
		{
			availableRectangle.setLeft(availableRectangle.left() + length);
		}
	}
}

QVector<AddressDelegate::TextSegment> AddressDelegate::highlightSegments(const QString &highlight, const QVector<TextSegment> &segments) const
{
	if (highlight.isEmpty())
	{
		return segments;
	}

	QVector<TextSegment> highlightedSegments;

	for (int i = 0; i < segments.count(); ++i)
	{
		const TextSegment segment(segments.at(i));
		const QStringList subSegments(segment.text.split(highlight, Qt::KeepEmptyParts, Qt::CaseInsensitive));
		int highlightAmount(0);

		for (int j = 0; j < subSegments.count(); ++j)
		{
			if (subSegments.at(j).isEmpty())
			{
				if (j > 0)
				{
					++highlightAmount;
				}

				if (j >= (subSegments.count() - 1) || !subSegments.at(j + 1).isEmpty())
				{
					highlightedSegments.append(TextSegment(highlight.repeated(highlightAmount), segment.color, true));

					highlightAmount = 0;

					continue;
				}
			}
			else if (j > 0)
			{
				highlightedSegments.append(TextSegment(highlight.repeated(highlightAmount + 1), segment.color, true));
			}

			highlightedSegments.append(TextSegment(subSegments.at(j), segment.color));
		}
	}

	return highlightedSegments;
}

QSize AddressDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(index.data(Qt::SizeHintRole).toSize());

	if (index.row() != 0 && static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::CompletionEntry::HeaderType)
	{
		size.setHeight(qRound(option.fontMetrics.lineSpacing() * 1.75));
	}
	else
	{
		size.setHeight(qRound(option.fontMetrics.lineSpacing() * 1.25));
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
	m_isSimplified(false),
	m_wasEdited(false)
{
	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (!toolBar)
	{
		m_isSimplified = true;
	}

	setPopupEntryRemovableRole(AddressCompletionModel::IsRemovableRole);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumWidth(100);
	setWindow(window);
	updateCompletion(true);
	handleOptionChanged(SettingsManager::AddressField_CompletionModeOption, SettingsManager::getOption(SettingsManager::AddressField_CompletionModeOption));
	handleOptionChanged(SettingsManager::AddressField_DropActionOption, SettingsManager::getOption(SettingsManager::AddressField_DropActionOption));
	handleOptionChanged(SettingsManager::AddressField_LayoutOption, SettingsManager::getOption(SettingsManager::AddressField_LayoutOption));
	handleOptionChanged(SettingsManager::AddressField_SelectAllOnFocusOption, SettingsManager::getOption(SettingsManager::AddressField_SelectAllOnFocusOption));

	if (toolBar)
	{
		setPlaceholderText(tr("Enter address or search…"));

		connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &AddressWidget::handleOptionChanged);

		if (toolBar->getDefinition().isGlobal())
		{
			connect(toolBar, &ToolBarWidget::windowChanged, this, &AddressWidget::setWindow);
		}
	}

	connect(this, &AddressWidget::requestedPopupEntryRemoval, [&](const QModelIndex &index)
	{
		if (index.isValid() && index.data(AddressCompletionModel::IsRemovableRole).toBool() && index.data(AddressCompletionModel::TypeRole).toInt() == AddressCompletionModel::CompletionEntry::TypedHistoryType)
		{
			HistoryManager::getTypedHistoryModel()->removeEntry(index.data(AddressCompletionModel::HistoryIdentifierRole).toULongLong());

			updateCompletion(true, true);
		}
	});
	connect(this, &AddressWidget::textEdited, this, [&]()
	{
		m_wasEdited = true;
	});
	connect(this, &AddressWidget::textDropped, this, [&](const QString &text)
	{
		handleUserInput(text);
	});
	connect(m_completionModel, &AddressCompletionModel::completionReady, this, &AddressWidget::setCompletion);
	connect(BookmarksManager::getModel(), &BookmarksModel::modelModified, this, [&]()
	{
		updateEntries({BookmarkEntry});
	});
}

void AddressWidget::changeEvent(QEvent *event)
{
	LineEditWidget::changeEvent(event);

	if (!m_isSimplified)
	{
		switch (event->type())
		{
			case QEvent::LanguageChange:
				updateEntries({WebsiteInformationEntry, ListFeedsEntry, BookmarkEntry, LoadPluginsEntry, FillPasswordEntry, HistoryDropdownEntry});
				setPlaceholderText(tr("Enter address or search…"));

				break;
			case QEvent::LayoutDirectionChange:
				updateGeometries();

				break;
			default:
				break;
		}
	}
}

void AddressWidget::paintEvent(QPaintEvent *event)
{
	LineEditWidget::paintEvent(event);

	QPainter painter(this);

	if (m_entries.contains(HistoryDropdownEntry))
	{
		QStyleOption dropdownArrowOption;
		dropdownArrowOption.initFrom(this);
		dropdownArrowOption.rect = m_entries[HistoryDropdownEntry].rectangle;

		if (HistoryManager::getTypedHistoryModel()->rowCount() == 0)
		{
			dropdownArrowOption.palette.setCurrentColorGroup(QPalette::Disabled);
		}

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &dropdownArrowOption, &painter, this);
	}

	if (m_isSimplified)
	{
		return;
	}

	QHash<EntryIdentifier, EntryDefinition>::const_iterator iterator;

	for (iterator = m_entries.constBegin(); iterator != m_entries.constEnd(); ++iterator)
	{
		if (iterator.value().isValid())
		{
			iterator.value().icon.paint(&painter, iterator.value().rectangle, Qt::AlignCenter, ((iterator.key() == m_hoveredEntry) ? QIcon::Active : QIcon::Normal));
		}
	}
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
			Application::triggerAction(ActionsManager::ActivateContentAction, {}, this);

			return;
		}
	}

	LineEditWidget::focusInEvent(event);

	activate(event->reason());
}

void AddressWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Down:
			if (!isPopupVisible() && HistoryManager::getTypedHistoryModel()->rowCount() > 0)
			{
				showCompletion(true);
			}

			break;
		case Qt::Key_Enter:
		case Qt::Key_Return:
			if (!m_isSimplified)
			{
				handleUserInput(text().trimmed(), SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen, Qt::LeftButton, event->modifiers()));
			}

			break;
		case Qt::Key_Escape:
			if (isPopupVisible())
			{
				hidePopup();
			}
			else if (m_window)
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

			break;
		default:
			break;
	}

	LineEditWidget::keyPressEvent(event);
}

void AddressWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const EntryIdentifier entry(getEntry(event->pos()));
	QMenu menu(this);

	if (entry == UnknownEntry || entry == AddressEntry)
	{
		ActionExecutor::Object executor(this, this);

		menu.addAction(new Action(ActionsManager::UndoAction, {}, executor, &menu));
		menu.addAction(new Action(ActionsManager::RedoAction, {}, executor, &menu));
		menu.addSeparator();
		menu.addAction(new Action(ActionsManager::CutAction, {}, executor, &menu));
		menu.addAction(new Action(ActionsManager::CopyAction, {}, executor, &menu));
		menu.addAction(new Action(ActionsManager::PasteAction, {}, executor, &menu));

		if (!m_isSimplified)
		{
			menu.addAction(new Action(ActionsManager::PasteAndGoAction, {}, ActionExecutor::Object(m_window, m_window), this));
		}

		menu.addAction(new Action(ActionsManager::DeleteAction, {}, executor, &menu));
		menu.addSeparator();
		menu.addAction(new Action(ActionsManager::CopyToNoteAction, {}, executor, &menu));
		menu.addSeparator();
		menu.addAction(new Action(ActionsManager::ClearAllAction, {}, executor, &menu));
		menu.addAction(new Action(ActionsManager::SelectAllAction, {}, executor, &menu));
	}
	else
	{
		const QUrl url(getUrl());

		if (entry == WebsiteInformationEntry && !Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
		{
			ActionExecutor::Object executor(m_window, m_window);

			menu.addAction(new Action(ActionsManager::WebsiteInformationAction, {}, executor, &menu));
			menu.addAction(new Action(ActionsManager::WebsitePreferencesAction, {}, executor, &menu));
			menu.addSeparator();
		}

		menu.addAction(ThemesManager::createIcon(QLatin1String("edit-delete")), tr("Remove Icon"), this, [&]()
		{
			const QString entryName(EnumeratorMapper(metaObject()->enumerator(m_entryIdentifierEnumerator), QLatin1String("Entry")).mapToName(entry));

			if (!entryName.isEmpty())
			{
				QStringList layout(SettingsManager::getOption(SettingsManager::AddressField_LayoutOption).toStringList());
				layout.removeAll(entryName);

				SettingsManager::setOption(SettingsManager::AddressField_LayoutOption, layout);
			}
		});
	}

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), {}, &menu));
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
		m_dragStartPosition = {};
	}

	LineEditWidget::mousePressEvent(event);
}

void AddressWidget::mouseMoveEvent(QMouseEvent *event)
{
	const QUrl url(getUrl());
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

		update();
	}

	if (event->buttons().testFlag(Qt::LeftButton) && !m_dragStartPosition.isNull() && (event->pos() - m_dragStartPosition).manhattanLength() >= QApplication::startDragDistance() && url.isValid())
	{
		Utils::startLinkDrag(url, (m_window ? m_window->getTitle() : QString()), ((m_window ? m_window->getIcon() : ThemesManager::createIcon(QLatin1String("tab"))).pixmap(16, 16)), this);
	}
	else
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
				if (m_window)
				{
					m_window->triggerAction(ActionsManager::WebsiteInformationAction);
				}

				event->accept();

				return;
			case ListFeedsEntry:
				{
					const QVector<WebWidget::LinkUrl> feeds((m_window && m_window->getLoadingState() == WebWidget::FinishedLoadingState && m_window->getWebWidget()) ? m_window->getWebWidget()->getFeeds() : QVector<WebWidget::LinkUrl>());

					if (feeds.count() == 1 && m_window)
					{
						m_window->setUrl(FeedsManager::createFeedReaderUrl(feeds.value(0).url));
					}
					else if (feeds.count() > 1)
					{
						QMenu menu;

						for (int i = 0; i < feeds.count(); ++i)
						{
							menu.addAction(feeds.at(i).title.isEmpty() ? tr("(Untitled)") : feeds.at(i).title)->setData(FeedsManager::createFeedReaderUrl(feeds.at(i).url));
						}

						connect(&menu, &QMenu::triggered, this, [&](QAction *action)
						{
							if (action && m_window)
							{
								m_window->setUrl(action->data().toUrl());
							}
						});

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
							const QVector<BookmarksModel::Bookmark*> bookmarks(BookmarksManager::getModel()->findUrls(url));

							for (int i = 0; i < bookmarks.count(); ++i)
							{
								BookmarksManager::getModel()->trashBookmark(bookmarks.at(i));
							}
						}
						else
						{
							QMenu menu;
							menu.addAction(tr("Add to Bookmarks"), this, [&]()
							{
								if (m_window)
								{
									BookmarkPropertiesDialog dialog(getUrl().adjusted(QUrl::RemovePassword), m_window->getTitle(), (m_window->getContentsWidget() ? m_window->getContentsWidget()->getDescription() : QString()), nullptr, -1, true, this);
									dialog.exec();
								}
							}, ActionsManager::getActionShortcut(ActionsManager::BookmarkPageAction))->setShortcutContext(Qt::WidgetShortcut);
							menu.addAction(tr("Add to Start Page"), this, [&]()
							{
								if (m_window)
								{
									BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, getUrl().adjusted(QUrl::RemovePassword)}, {BookmarksModel::TitleRole, m_window->getTitle()}}, BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString(), true));
								}
							});
							menu.exec(mapToGlobal(m_entries.value(BookmarkEntry).rectangle.bottomLeft()));
						}

						updateEntries({BookmarkEntry});
					}

					event->accept();
				}

				return;
			case LoadPluginsEntry:
				m_window->triggerAction(ActionsManager::LoadPluginsAction);

				updateEntries({LoadPluginsEntry});

				event->accept();

				return;
			case FillPasswordEntry:
				m_window->triggerAction(ActionsManager::FillPasswordAction);

				event->accept();

				return;
			case HistoryDropdownEntry:
				if (!isPopupVisible() && HistoryManager::getTypedHistoryModel()->rowCount() > 0)
				{
					showCompletion(true);
				}

				break;
			default:
				break;
		}
	}

	if (event->button() == Qt::LeftButton && !isPopupVisible() && SettingsManager::getOption(SettingsManager::AddressField_ShowSuggestionsOnFocusOption).toBool() && HistoryManager::getTypedHistoryModel()->rowCount() > 0)
	{
		showCompletion(true);
	}
	else if (event->button() == Qt::MiddleButton && text().isEmpty() && !QGuiApplication::clipboard()->text().isEmpty() && SettingsManager::getOption(SettingsManager::AddressField_PasteAndGoOnMiddleClickOption).toBool())
	{
		handleUserInput(QGuiApplication::clipboard()->text().trimmed(), SessionsManager::CurrentTabOpen);

		event->accept();
	}

	m_clickedEntry = UnknownEntry;

	LineEditWidget::mouseReleaseEvent(event);
}

void AddressWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		event->accept();
	}

	LineEditWidget::dragEnterEvent(event);
}

void AddressWidget::showCompletion(bool isTypedHistory)
{
	PopupViewWidget *popupWidget(getPopup());
	popupWidget->setModel(m_completionModel);
	popupWidget->setItemDelegate(new AddressDelegate((isTypedHistory ? QString() : text()), (isTypedHistory ? AddressDelegate::HistoryMode : AddressDelegate::CompletionMode), popupWidget));

	updateCompletion(isTypedHistory, isTypedHistory);

	if (!isPopupVisible())
	{
		connect(popupWidget, &PopupViewWidget::clicked, this, [&](const QModelIndex &index)
		{
			hidePopup();

			if (index.isValid())
			{
				if (static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::CompletionEntry::SearchSuggestionType)
				{
					emit requestedSearch(index.data(AddressCompletionModel::TextRole).toString(), SearchEnginesManager::getSearchEngine(index.data(AddressCompletionModel::KeywordRole).toString(), true).identifier, SessionsManager::CurrentTabOpen);
				}
				else
				{
					const QString url(index.data(AddressCompletionModel::UrlRole).toUrl().toString());

					setUrl(url);
					handleUserInput(url, SessionsManager::CurrentTabOpen);
				}
			}
		});
		connect(popupWidget, &PopupViewWidget::customContextMenuRequested, this, [&](const QPoint &position)
		{
			const QModelIndex index(getPopup()->indexAt(position));

			if (!index.isValid())
			{
				return;
			}

			const AddressCompletionModel::CompletionEntry::EntryType type(static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()));

			if (type <= AddressCompletionModel::CompletionEntry::HeaderType || type == AddressCompletionModel::CompletionEntry::SearchSuggestionType)
			{
				return;
			}

			QMenu menu(this);
			menu.addAction(ThemesManager::createIcon(QLatin1String("edit-copy")), QCoreApplication::translate("actons", "Copy"), this, [&]()
			{
				QGuiApplication::clipboard()->setText(index.data(AddressCompletionModel::UrlRole).toUrl().toString());
			});

			if (isTypedHistory && index.data(AddressCompletionModel::IsRemovableRole).toBool() && type == AddressCompletionModel::CompletionEntry::TypedHistoryType)
			{
				menu.addSeparator();
				menu.addAction(ThemesManager::createIcon(QLatin1String("edit-delete")), tr("Remove Entry"), this, [&]()
				{
					HistoryManager::getTypedHistoryModel()->removeEntry(index.data(AddressCompletionModel::HistoryIdentifierRole).toULongLong());

					updateCompletion(true, true);
				});
			}

			menu.exec(getPopup()->mapToGlobal(position));
		});
		connect(popupWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, [&](const QModelIndex &index)
		{
			if (m_isNavigatingCompletion)
			{
				m_isNavigatingCompletion = false;

				setText(index.data(AddressCompletionModel::TextRole).toString());
			}
		});

		showPopup();
	}

	popupWidget->setCurrentIndex(m_completionModel->index(0, 0));
	popupWidget->setFocus();
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

				disconnect(this, &AddressWidget::textEdited, m_completionModel, &AddressCompletionModel::setFilter);

				if (m_completionModes != NoCompletionMode)
				{
					connect(this, &AddressWidget::textEdited, m_completionModel, &AddressCompletionModel::setFilter);
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
			if (m_isSimplified)
			{
				m_layout = {AddressEntry, HistoryDropdownEntry};

				updateGeometries();
			}
			else
			{
				if (m_entryIdentifierEnumerator < 0)
				{
					m_entryIdentifierEnumerator = metaObject()->indexOfEnumerator("EntryIdentifier");
				}

				const EnumeratorMapper enumeratorMapper(metaObject()->enumerator(m_entryIdentifierEnumerator), QLatin1String("Entry"));
				const QStringList rawLayout(value.toStringList());
				const QVector<EntryIdentifier> previousLayout(m_layout);
				QVector<EntryIdentifier> layout;
				layout.reserve(rawLayout.count());

				for (int i = 0; i < rawLayout.count(); ++i)
				{
					const EntryIdentifier entryIdentifier(static_cast<EntryIdentifier>(enumeratorMapper.mapToValue(rawLayout.at(i).toLatin1())));

					if (entryIdentifier > UnknownEntry && !layout.contains(entryIdentifier))
					{
						layout.append(entryIdentifier);
					}
				}

				if (!layout.contains(AddressEntry))
				{
					layout.prepend(AddressEntry);
				}

				m_layout = layout;

				updateEntries(previousLayout + layout);
			}

			break;
		default:
			break;
	}
}

void AddressWidget::handleActionsStateChanged(const QVector<int> &identifiers)
{
	if (identifiers.contains(ActionsManager::LoadPluginsAction) && m_layout.contains(LoadPluginsEntry))
	{
		updateEntries({LoadPluginsEntry});
	}
}

void AddressWidget::handleIconChanged()
{
	updateEntries({FaviconEntry});
}

void AddressWidget::handleWatchedDataChanged(WebWidget::ChangeWatcher watcher)
{
	if (watcher == WebWidget::FeedsWatcher)
	{
		updateEntries({ListFeedsEntry});
	}
}

void AddressWidget::handleUserInput(const QString &text, SessionsManager::OpenHints hints)
{
	if (m_isSimplified || text.isEmpty())
	{
		return;
	}

	if (hints == SessionsManager::DefaultOpen)
	{
		hints = SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen);
	}

	const InputInterpreter::InterpreterResult result(InputInterpreter::interpret(text));

	if (!result.isValid())
	{
		return;
	}

	MainWindow *mainWindow(m_window ? MainWindow::findMainWindow(m_window) : MainWindow::findMainWindow(this));
	ActionExecutor::Object executor(mainWindow, mainWindow);

	switch (result.type)
	{
		case InputInterpreter::InterpreterResult::BookmarkType:
			if (executor.isValid())
			{
				executor.triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), result.bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(hints)}});
			}

			break;
		case InputInterpreter::InterpreterResult::UrlType:
			if (executor.isValid())
			{
				executor.triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), result.url}, {QLatin1String("hints"), QVariant(hints)}});
			}

			break;
		case InputInterpreter::InterpreterResult::SearchType:
			emit requestedSearch(result.searchQuery, result.searchEngine, hints);

			break;
		default:
			break;
	}
}

void AddressWidget::updateEntries(const QVector<EntryIdentifier> &identifiers)
{
	const QUrl url(getUrl());
	const bool hasValidWindow(m_window && !m_window->isAboutToClose() && m_window->getLoadingState() == WebWidget::FinishedLoadingState);
	bool needsUpdate(false);
	bool needsRepaint(false);

	for (int i = 0; i < identifiers.count(); ++i)
	{
		const EntryIdentifier identifier(identifiers.at(i));
		const bool isUpdating(m_entries.contains(identifier));

		if (!m_layout.contains(identifier))
		{
			if (isUpdating)
			{
				m_entries.remove(identifier);

				needsUpdate = true;
			}

			continue;
		}

		const QString previousIconName(m_entries.value(identifier).iconName);
		EntryDefinition definition;
		definition.identifier = identifier;

		switch (identifier)
		{
			case WebsiteInformationEntry:
				{
					const WebWidget::ContentStates state(m_window ? m_window->getContentState() : WebWidget::UnknownContentState);

					if (state.testFlag(WebWidget::FraudContentState))
					{
						definition.iconName = QLatin1String("badge-fraud");
					}
					else if (state.testFlag(WebWidget::AmbiguousContentState))
					{
						definition.iconName = QLatin1String("badge-ambiguous");
					}
					else if (state.testFlag(WebWidget::MixedContentState))
					{
						definition.iconName = QLatin1String("badge-mixed");
					}
					else if (state.testFlag(WebWidget::SecureContentState))
					{
						definition.iconName = QLatin1String("badge-secure");
					}
					else if (state.testFlag(WebWidget::RemoteContentState))
					{
						definition.iconName = QLatin1String("badge-remote");
					}
					else if (state.testFlag(WebWidget::LocalContentState))
					{
						definition.iconName = QLatin1String("badge-local");
					}
					else if (state.testFlag(WebWidget::ApplicationContentState))
					{
						definition.iconName = QLatin1String("otter-browser");
					}
					else
					{
						definition.iconName = QLatin1String("unknown");
					}

					if (!Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
					{
						definition.title = QT_TR_NOOP("Show website information");
					}
				}

				break;
			case FaviconEntry:
				if (m_window)
				{
					definition.icon = m_window->getIcon();

					needsRepaint = true;
				}
				else
				{
					definition.iconName = (SessionsManager::isPrivate() ? QLatin1String("tab-private") : QLatin1String("tab"));
				}

				break;
			case ListFeedsEntry:
				if (m_window && m_window->getWebWidget() && !m_window->getWebWidget()->getFeeds().isEmpty())
				{
					definition.title = QT_TR_NOOP("Show feed list");
					definition.iconName = QLatin1String("application-rss+xml");
				}

				break;
			case BookmarkEntry:
				if (!Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
				{
					if (BookmarksManager::hasBookmark(url))
					{
						definition.title = QT_TR_NOOP("Remove bookmark");
						definition.iconName = QLatin1String("bookmark-page-remove");
					}
					else
					{
						definition.title = QT_TR_NOOP("Add bookmark");
						definition.iconName = QLatin1String("bookmark-page-new");
					}
				}

				break;
			case LoadPluginsEntry:
				if (hasValidWindow && m_window->getActionState(ActionsManager::LoadPluginsAction).isEnabled)
				{
					definition.title = QT_TR_NOOP("Load all plugins on the page");
					definition.iconName = QLatin1String("preferences-plugin");
				}

				break;
			case FillPasswordEntry:
				if (hasValidWindow && !Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about") && PasswordsManager::hasPasswords(url, PasswordsManager::FormPassword))
				{
					definition.title = QT_TR_NOOP("Log in");
					definition.iconName = QLatin1String("fill-password");
				}

				break;
			default:
				break;
		}

		if (identifier == HistoryDropdownEntry || definition.isValid())
		{
			if (definition.icon.isNull() || definition.iconName != previousIconName)
			{
				needsRepaint = true;

				if (!definition.iconName.isEmpty())
				{
					definition.icon = ThemesManager::createIcon(definition.iconName, false);
				}
			}

			m_entries[identifier] = definition;

			if (!isUpdating || definition.rectangle.isNull())
			{
				needsUpdate = true;
			}
		}
		else if (isUpdating)
		{
			m_entries.remove(identifier);

			needsUpdate = true;
		}
	}

	if (needsUpdate)
	{
		updateGeometries();
	}
	else if (needsRepaint)
	{
		update();
	}
}

void AddressWidget::updateCurrentEntries()
{
	updateEntries(m_layout);
}

void AddressWidget::updateGeometries()
{
	QVector<EntryDefinition> leadingEntries;
	QVector<EntryDefinition> trailingEntries;
	const int offset(qMax(((height() - 16) / 2), 2));
	QMargins margins(offset, 0, offset, 0);
	int availableWidth(width() - margins.left() - margins.right());
	const bool isRightToLeft(layoutDirection() == Qt::RightToLeft);
	bool isLeading(!isRightToLeft);

	if (m_layout.contains(WebsiteInformationEntry))
	{
		availableWidth -= 20;
	}

	if (m_layout.contains(HistoryDropdownEntry))
	{
		availableWidth -= 16;
	}

	for (int i = 0; i < m_layout.count(); ++i)
	{
		if (m_layout.at(i) == AddressEntry)
		{
			isLeading = !isLeading;
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

		const EntryDefinition definition(m_entries.value(m_layout.at(i)));

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
				leadingEntries[i].rectangle = {margins.left(), ((height() - 16) / 2), 16, 16};

				margins.setLeft(margins.left() + 20);

				break;
			case HistoryDropdownEntry:
				leadingEntries[i].rectangle = {margins.left(), 0, 14, height()};

				margins.setLeft(margins.left() + 16);

				break;
			default:
				break;
		}

		m_entries[leadingEntries.at(i).identifier].rectangle = leadingEntries.at(i).rectangle;
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
				trailingEntries[i].rectangle = {(width() - margins.right() - 20), ((height() - 16) / 2), 16, 16};

				margins.setRight(margins.right() + 20);

				break;
			case HistoryDropdownEntry:
				trailingEntries[i].rectangle = {(width() - margins.right() - 14), 0, 14, height()};

				margins.setRight(margins.right() + 16);

				break;
			default:
				break;
		}

		m_entries[trailingEntries.at(i).identifier].rectangle = trailingEntries.at(i).rectangle;
	}

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

void AddressWidget::updateCompletion(bool isTypedHistory, bool force)
{
	AddressCompletionModel::CompletionTypes types(AddressCompletionModel::NoCompletionType);

	if (isTypedHistory)
	{
		types = AddressCompletionModel::TypedHistoryCompletionType;
	}
	else
	{
		if (SettingsManager::getOption(SettingsManager::AddressField_SuggestBookmarksOption).toBool())
		{
			types |= AddressCompletionModel::BookmarksCompletionType;
		}

		if (SettingsManager::getOption(SettingsManager::AddressField_SuggestHistoryOption).toBool())
		{
			types |= AddressCompletionModel::HistoryCompletionType;
		}

		if (!m_isSimplified && SettingsManager::getOption(SettingsManager::AddressField_SuggestSearchOption).toBool())
		{
			types |= AddressCompletionModel::SearchSuggestionsCompletionType;
		}

		if (SettingsManager::getOption(SettingsManager::AddressField_SuggestSpecialPagesOption).toBool())
		{
			types |= AddressCompletionModel::SpecialPagesCompletionType;
		}

		if (SettingsManager::getOption(SettingsManager::AddressField_SuggestLocalPathsOption).toBool())
		{
			types |= AddressCompletionModel::LocalPathSuggestionsCompletionType;
		}
	}

	m_completionModel->setTypes(types, force);
}

void AddressWidget::setCompletion(const QString &filter)
{
	if (filter.isEmpty() || m_completionModel->rowCount() == 0)
	{
		hidePopup();

		LineEditWidget::setCompletion({});

		return;
	}

	if (m_completionModes.testFlag(PopupCompletionMode))
	{
		showCompletion(false);
	}

	if (m_completionModes.testFlag(InlineCompletionMode))
	{
		for (int i = 0; i < m_completionModel->rowCount(); ++i)
		{
			const QString matchedText(m_completionModel->index(i).data(AddressCompletionModel::MatchRole).toString());

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
	const MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (m_window && !m_window->isAboutToClose() && (!sender() || sender() != m_window))
	{
		disconnect(this, &AddressWidget::requestedSearch, m_window.data(), &Window::requestedSearch);
		disconnect(m_window.data(), &Window::urlChanged, this, &AddressWidget::setUrl);
		disconnect(m_window.data(), &Window::iconChanged, this, &AddressWidget::handleIconChanged);
		disconnect(m_window.data(), &Window::arbitraryActionsStateChanged, this, &AddressWidget::handleActionsStateChanged);
		disconnect(m_window.data(), &Window::contentStateChanged, this, &AddressWidget::updateCurrentEntries);
		disconnect(m_window.data(), &Window::loadingStateChanged, this, &AddressWidget::updateCurrentEntries);
		disconnect(m_window->getMainWindow(), &MainWindow::activeWindowChanged, this, &AddressWidget::hidePopup);

		if (m_window->getWebWidget())
		{
			m_window->getWebWidget()->stopWatchingChanges(this, WebWidget::FeedsWatcher);

			disconnect(m_window->getWebWidget(), &WebWidget::watchedDataChanged, this, &AddressWidget::handleWatchedDataChanged);
		}
	}

	m_window = window;

	if (window)
	{
		if (mainWindow)
		{
			disconnect(this, &AddressWidget::requestedSearch, mainWindow, &MainWindow::search);
		}

		if (isVisible() && window->isActive() && Utils::isUrlEmpty(window->getUrl()))
		{
			const AddressWidget *addressWidget(qobject_cast<AddressWidget*>(QApplication::focusWidget()));

			if (!addressWidget)
			{
				setFocus();
			}
		}

		connect(this, &AddressWidget::requestedSearch, window, &Window::requestedSearch);
		connect(window, &Window::urlChanged, this, &AddressWidget::setUrl);
		connect(window, &Window::iconChanged, this, &AddressWidget::handleIconChanged);
		connect(window, &Window::arbitraryActionsStateChanged, this, &AddressWidget::handleActionsStateChanged);
		connect(window, &Window::contentStateChanged, this, &AddressWidget::updateCurrentEntries);
		connect(window, &Window::loadingStateChanged, this, &AddressWidget::updateCurrentEntries);
		connect(window->getMainWindow(), &MainWindow::activeWindowChanged, this, &AddressWidget::hidePopup);
		connect(window, &Window::destroyed, this, [&](QObject *object)
		{
			if (qobject_cast<Window*>(object) == m_window)
			{
				setWindow(nullptr);
			}
		});

		WebWidget *webWidget(window->getWebWidget());

		if (webWidget)
		{
			webWidget->startWatchingChanges(this, WebWidget::FeedsWatcher);

			connect(webWidget, &WebWidget::watchedDataChanged, this, &AddressWidget::handleWatchedDataChanged);
		}

		const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

		if (!toolBar || toolBar->getDefinition().isGlobal())
		{
			connect(window, &Window::aboutToClose, this, [&]()
			{
				setWindow(nullptr);
			});
		}
	}
	else if (mainWindow && !mainWindow->isAboutToClose() && !m_isSimplified)
	{
		connect(this, &AddressWidget::requestedSearch, mainWindow, &MainWindow::search);
	}

	updateEntries({FaviconEntry});
	setUrl(window ? window->getUrl() : QUrl());
}

void AddressWidget::setUrl(const QUrl &url, bool force)
{
	const QString text(Utils::isUrlEmpty(url) ? QString() : url.toString());

	if (!m_isSimplified)
	{
		updateCurrentEntries();
	}

	if (m_isSimplified || ((force || !m_wasEdited || !hasFocus()) && url.scheme() != QLatin1String("javascript")))
	{
		const QString host(url.host(QUrl::FullyEncoded));

		setToolTip((host == url.host()) ? text : QStringLiteral("(%1) %2").arg(host, text));
		setText(text);
		setCursorPosition(0);

		m_wasEdited = false;
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

bool AddressWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		const EntryIdentifier entry(getEntry(helpEvent->pos()));

		if (entry != UnknownEntry && entry != AddressEntry)
		{
			QString toolTip;
			QKeySequence shortcut;
			const QString title(m_entries[entry].title);

			if (!title.isEmpty())
			{
				toolTip = tr(title.toUtf8().constData());
			}

			switch (entry)
			{
				case HistoryDropdownEntry:
					shortcut = ActionsManager::getActionShortcut(ActionsManager::ActivateAddressFieldAction, {{QLatin1String("showTypedHistoryDropdown"), true}});

					break;
				case WebsiteInformationEntry:
					shortcut = ActionsManager::getActionShortcut(ActionsManager::WebsiteInformationAction);

					break;
				default:
					break;
			}

			if (!shortcut.isEmpty())
			{
				if (!toolTip.isEmpty())
				{
					toolTip.append(QLatin1Char(' '));
				}

				toolTip.append(QLatin1Char('(') + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')'));
			}

			if (!toolTip.isEmpty())
			{
				QToolTip::showText(helpEvent->globalPos(), toolTip);

				return true;
			}
		}
	}

	return LineEditWidget::event(event);
}

}
