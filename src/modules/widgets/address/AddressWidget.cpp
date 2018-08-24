/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/ActionsManager.h"
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
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtCore/QMetaEnum>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocument>
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
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &AddressDelegate::handleOptionChanged);
}

void AddressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QRect titleRectangle(option.rect);
	const bool isRightToLeft(option.direction == Qt::RightToLeft);

	if (static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::CompletionEntry::HeaderType)
	{
		QStyleOptionViewItem headerOption(option);
		headerOption.rect = titleRectangle.marginsRemoved(QMargins(0, 2, (isRightToLeft ? 3 : 0), 2));
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

	QAbstractTextDocumentLayout::PaintContext paintContext;
	QTextDocument document;
	document.setDefaultFont(option.font);

	QString url(index.data(Qt::DisplayRole).toString());
	QString description((m_viewMode == HistoryMode) ? Utils::formatDateTime(index.data(AddressCompletionModel::TimeVisitedRole).toDateTime()) : index.data(AddressCompletionModel::TitleRole).toString());
	const int topPosition(titleRectangle.top() - qRound((titleRectangle.height() - painter->clipBoundingRect().united(document.documentLayout()->blockBoundingRect(document.firstBlock())).height()) / 2));
	const bool isSearchSuggestion(static_cast<AddressCompletionModel::CompletionEntry::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::CompletionEntry::SearchSuggestionType);

	if (option.state.testFlag(QStyle::State_Selected))
	{
		painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));

		paintContext.palette.setColor(QPalette::Text, option.palette.color(QPalette::HighlightedText));
	}
	else if (!isSearchSuggestion)
	{
		paintContext.palette.setColor(QPalette::Text, option.palette.color(QPalette::Link));
	}

	QRect decorationRectangle(option.rect);

	if (isRightToLeft)
	{
		decorationRectangle.setLeft(option.rect.width() - option.rect.height() - 5);

		titleRectangle.setRight(option.rect.width() - option.rect.height() - 10);
	}
	else
	{
		decorationRectangle.setRight(option.rect.height());

		titleRectangle.setLeft(option.rect.height());
	}

	decorationRectangle = decorationRectangle.marginsRemoved(QMargins(2, 2, 2, 2));

	QIcon icon(index.data(Qt::DecorationRole).value<QIcon>());

	if (icon.isNull())
	{
		icon = ThemesManager::createIcon(QLatin1String("tab"));
	}

	icon.paint(painter, decorationRectangle, option.decorationAlignment);

	if (m_displayMode == ColumnsMode)
	{
		const int maxUrlWidth(option.rect.width() / 2);

		url = option.fontMetrics.elidedText(url, Qt::ElideRight, (maxUrlWidth - 40));

		painter->save();

		if (isRightToLeft)
		{
			painter->translate((titleRectangle.right() - calculateLength(option, url)), topPosition);
		}
		else
		{
			painter->translate(titleRectangle.left(), topPosition);
		}

		document.setHtml(isSearchSuggestion ? url : highlightText(url));
		document.documentLayout()->draw(painter, paintContext);

		painter->restore();

		if (!description.isEmpty())
		{
			painter->save();

			description = option.fontMetrics.elidedText(description, (isRightToLeft ? Qt::ElideLeft : Qt::ElideRight), (maxUrlWidth - 10));

			if (isRightToLeft)
			{
				titleRectangle.setRight(maxUrlWidth);

				painter->translate((titleRectangle.right() - calculateLength(option, description)), topPosition);
			}
			else
			{
				titleRectangle.setLeft(maxUrlWidth);

				painter->translate(titleRectangle.left(), topPosition);
			}

			document.setHtml(highlightText(description));

			if (option.state.testFlag(QStyle::State_Selected))
			{
				document.documentLayout()->draw(painter, paintContext);
			}
			else
			{
				document.drawContents(painter);
			}

			painter->restore();
		}

		return;
	}

	painter->save();

	url = option.fontMetrics.elidedText(url, Qt::ElideRight, (option.rect.width() - 40));

	if (isRightToLeft)
	{
		painter->translate((titleRectangle.right() - calculateLength(option, url)), topPosition);
	}
	else
	{
		painter->translate(titleRectangle.left(), topPosition);
	}

	document.setHtml(isSearchSuggestion ? url : highlightText(url));
	document.documentLayout()->draw(painter, paintContext);

	painter->restore();

	if (!description.isEmpty())
	{
		const int urlLength(calculateLength(option, url + QLatin1Char(' ')));

		if ((urlLength + 40) < titleRectangle.width())
		{
			painter->save();

			description = option.fontMetrics.elidedText(description, (isRightToLeft ? Qt::ElideLeft : Qt::ElideRight), (option.rect.width() - urlLength - 50));

			if (isRightToLeft)
			{
				description.append(QLatin1String(" -"));

				titleRectangle.setRight(option.rect.width() - calculateLength(option, description) - (urlLength + 33));

				painter->translate(titleRectangle.right(), topPosition);
			}
			else
			{
				description.insert(0, QLatin1String("- "));

				titleRectangle.setLeft(urlLength + 33);

				painter->translate(titleRectangle.left(), topPosition);
			}

			document.setHtml(highlightText(description));

			if (option.state.testFlag(QStyle::State_Selected))
			{
				document.documentLayout()->draw(painter, paintContext);
			}
			else
			{
				document.drawContents(painter);
			}

			painter->restore();
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

QString AddressDelegate::highlightText(const QString &text, const QString &html) const
{
	const int index(text.indexOf(m_highlight, 0, Qt::CaseInsensitive));

	if (m_highlight.isEmpty() || index < 0)
	{
		return (html + text);
	}

	return highlightText(text.mid(index + m_highlight.length()), html + text.left(index) + QStringLiteral("<b>%1</b>").arg(text.mid(index, m_highlight.length())));
}

int AddressDelegate::calculateLength(const QStyleOptionViewItem &option, const QString &text, int length) const
{
	const int index(text.indexOf(m_highlight, 0, Qt::CaseInsensitive));

	if (m_highlight.isEmpty() || index < 0)
	{
		return (length + option.fontMetrics.width(text));
	}

	length += option.fontMetrics.width(text.left(index));

	QStyleOptionViewItem highlightedOption(option);
	highlightedOption.font.setBold(true);

	length += highlightedOption.fontMetrics.width(text.mid(index, m_highlight.length()));

	return calculateLength(option, text.mid(index + m_highlight.length()), length);
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
	m_hasFeeds(false),
	m_isNavigatingCompletion(false),
	m_isUsingSimpleMode(false),
	m_wasEdited(false)
{
	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (!toolBar)
	{
		m_isUsingSimpleMode = true;
	}

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

	connect(this, &AddressWidget::textEdited, this, [&]()
	{
		m_wasEdited = true;
	});
	connect(this, &AddressWidget::textDropped, this, [&](const QString &text)
	{
		handleUserInput(text);
	});
	connect(m_completionModel, &AddressCompletionModel::completionReady, this, &AddressWidget::setCompletion);
	connect(BookmarksManager::getModel(), &BookmarksModel::modelModified, this, &AddressWidget::updateGeometries);
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
		QStyleOption arrowOption;
		arrowOption.initFrom(this);
		arrowOption.rect = m_entries[HistoryDropdownEntry].rectangle;

		if (HistoryManager::getTypedHistoryModel()->rowCount() == 0)
		{
			arrowOption.palette.setCurrentColorGroup(QPalette::Disabled);
		}

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOption, &painter, this);
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
			if (!m_isUsingSimpleMode)
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

		if (!m_isUsingSimpleMode)
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

		menu.addAction(tr("Remove this Icon"), this, &AddressWidget::removeEntry)->setData(entry);
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
						m_window->setUrl(QUrl(QLatin1String("view-feed:") + feeds.first().url.toDisplayString()));
					}
					else if (feeds.count() > 1)
					{
						QMenu menu;

						for (int i = 0; i < feeds.count(); ++i)
						{
							menu.addAction(feeds.at(i).title.isEmpty() ? tr("(Untitled)") : feeds.at(i).title)->setData(QUrl(QLatin1String("view-feed:") + feeds.at(i).url.toDisplayString()));
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
							BookmarksManager::removeBookmark(url);
						}
						else
						{
							QMenu menu;
							QAction *addBookmarkAction(menu.addAction(tr("Add to Bookmarks"), [&]()
							{
								if (m_window)
								{
									BookmarkPropertiesDialog dialog(getUrl().adjusted(QUrl::RemovePassword), m_window->getTitle(), (m_window->getContentsWidget() ? m_window->getContentsWidget()->getDescription() : QString()), nullptr, -1, true, this);
									dialog.exec();
								}
							}));
							addBookmarkAction->setShortcut(ActionsManager::getActionShortcut(ActionsManager::BookmarkPageAction));
							addBookmarkAction->setShortcutContext(Qt::WidgetShortcut);

							menu.addAction(tr("Add to Start Page"), [&]()
							{
								if (m_window)
								{
									BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, getUrl().adjusted(QUrl::RemovePassword)}, {BookmarksModel::TitleRole, m_window->getTitle()}}, BookmarksManager::getModel()->getBookmarkByPath(SettingsManager::getOption(SettingsManager::StartPage_BookmarksFolderOption).toString()));
								}
							});

							menu.exec(mapToGlobal(m_entries.value(BookmarkEntry).rectangle.bottomLeft()));
						}

						updateGeometries();
					}

					event->accept();
				}

				return;
			case LoadPluginsEntry:
				m_window->triggerAction(ActionsManager::LoadPluginsAction);

				updateGeometries();

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

	if (event->button() == Qt::MiddleButton && text().isEmpty() && !QApplication::clipboard()->text().isEmpty() && SettingsManager::getOption(SettingsManager::AddressField_PasteAndGoOnMiddleClickOption).toBool())
	{
		handleUserInput(QApplication::clipboard()->text().trimmed(), SessionsManager::CurrentTabOpen);

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

void AddressWidget::openUrl(const QString &url)
{
	setUrl(url);
	handleUserInput(url, SessionsManager::CurrentTabOpen);
}

void AddressWidget::removeEntry()
{
	const QAction *action(qobject_cast<QAction*>(sender()));

	if (action)
	{
		QStringList layout(SettingsManager::getOption(SettingsManager::AddressField_LayoutOption).toStringList());
		QString name(metaObject()->enumerator(m_entryIdentifierEnumerator).valueToKey(action->data().toInt()));

		if (!name.isEmpty())
		{
			name.chop(5);
			name[0] = name.at(0).toLower();

			layout.removeAll(name);

			SettingsManager::setOption(SettingsManager::AddressField_LayoutOption, layout);
		}
	}
}

void AddressWidget::showCompletion(bool isTypedHistory)
{
	PopupViewWidget *popupWidget(getPopup());
	popupWidget->setModel(m_completionModel);
	popupWidget->setItemDelegate(new AddressDelegate((isTypedHistory ? QString() : text()), (isTypedHistory ? AddressDelegate::HistoryMode : AddressDelegate::CompletionMode), popupWidget));

	updateCompletion(isTypedHistory);

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

					const EntryIdentifier entryIdentifier(static_cast<EntryIdentifier>(metaObject()->enumerator(m_entryIdentifierEnumerator).keyToValue(name.toLatin1())));

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
			}

			updateGeometries();

			break;
		default:
			break;
	}
}

void AddressWidget::handleActionsStateChanged(const QVector<int> &identifiers)
{
	if (identifiers.contains(ActionsManager::LoadPluginsAction) && m_layout.contains(LoadPluginsEntry))
	{
		updateGeometries();
	}
}

void AddressWidget::handleWatchedDataChanged(WebWidget::ChangeWatcher watcher)
{
	if (watcher == WebWidget::FeedsWatcher)
	{
		m_hasFeeds = (m_window && m_window->getWebWidget() && !m_window->getWebWidget()->getFeeds().isEmpty());

		updateGeometries();
	}
}

void AddressWidget::handleLoadingStateChanged()
{
	m_hasFeeds = (m_window && m_window->getWebWidget() && !m_window->getWebWidget()->getFeeds().isEmpty());

	updateGeometries();
}

void AddressWidget::handleUserInput(const QString &text, SessionsManager::OpenHints hints)
{
	if (m_isUsingSimpleMode)
	{
		return;
	}

	if (hints == SessionsManager::DefaultOpen)
	{
		hints = SessionsManager::calculateOpenHints(SessionsManager::CurrentTabOpen);
	}

	if (!text.isEmpty())
	{
		const InputInterpreter::InterpreterResult result(InputInterpreter::interpret(text));

		if (result.isValid())
		{
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
	}
}

void AddressWidget::updateGeometries()
{
	QHash<EntryIdentifier, EntryDefinition> entries;
	QVector<EntryDefinition> leadingEntries;
	QVector<EntryDefinition> trailingEntries;
	const int offset(qMax(((height() - 16) / 2), 2));
	QMargins margins(offset, 0, offset, 0);
	const QUrl url(getUrl());
	int availableWidth(width() - margins.left() - margins.right());
	const bool hasValidWindow(m_window && !m_window->isAboutToClose() && m_window->getLoadingState() == WebWidget::FinishedLoadingState);
	bool isLeading(true);
	const bool isRightToLeft(layoutDirection() == Qt::RightToLeft);

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

					definition.icon = ThemesManager::createIcon(icon, false);
				}

				break;
			case FaviconEntry:
				definition.icon = (m_window ? m_window->getIcon() : ThemesManager::createIcon((SessionsManager::isPrivate() ? QLatin1String("tab-private") : QLatin1String("tab")), false));

				break;
			case ListFeedsEntry:
				if (m_hasFeeds)
				{
					definition.title = QT_TR_NOOP("Show feed list");
					definition.icon = ThemesManager::createIcon(QLatin1String("application-rss+xml"), false);
				}

				break;
			case BookmarkEntry:
				if (!Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about"))
				{
					definition.icon = ThemesManager::createIcon(QLatin1String("bookmarks"), false);

					if (BookmarksManager::hasBookmark(url))
					{
						definition.title = QT_TR_NOOP("Remove bookmark");
						definition.mode = QIcon::Normal;
					}
					else
					{
						definition.title = QT_TR_NOOP("Add bookmark");
						definition.mode = QIcon::Disabled;
					}
				}

				break;
			case LoadPluginsEntry:
				if (hasValidWindow && m_window->getActionState(ActionsManager::LoadPluginsAction).isEnabled)
				{
					definition.title = QT_TR_NOOP("Load all plugins on the page");
					definition.icon = ThemesManager::createIcon(QLatin1String("preferences-plugin"), false);
				}

				break;
			case FillPasswordEntry:
				if (hasValidWindow && !Utils::isUrlEmpty(url) && url.scheme() != QLatin1String("about") && PasswordsManager::hasPasswords(url, PasswordsManager::FormPassword))
				{
					definition.title = QT_TR_NOOP("Log in");
					definition.icon = ThemesManager::createIcon(QLatin1String("fill-password"), false);
				}

				break;
			default:
				break;
		}

		if (m_layout.at(i) != HistoryDropdownEntry && definition.icon.isNull())
		{
			continue;
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

void AddressWidget::updateCompletion(bool isTypedHistory)
{
	AddressCompletionModel::CompletionTypes types(AddressCompletionModel::UnknownCompletionType);

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

		if (!m_isUsingSimpleMode && SettingsManager::getOption(SettingsManager::AddressField_SuggestSearchOption).toBool())
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

	m_completionModel->setTypes(types);
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
		disconnect(m_window.data(), &Window::iconChanged, this, &AddressWidget::setIcon);
		disconnect(m_window.data(), &Window::arbitraryActionsStateChanged, this, &AddressWidget::handleActionsStateChanged);
		disconnect(m_window.data(), &Window::contentStateChanged, this, &AddressWidget::updateGeometries);
		disconnect(m_window.data(), &Window::loadingStateChanged, this, &AddressWidget::handleLoadingStateChanged);

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
		connect(window, &Window::iconChanged, this, &AddressWidget::setIcon);
		connect(window, &Window::arbitraryActionsStateChanged, this, &AddressWidget::handleActionsStateChanged);
		connect(window, &Window::contentStateChanged, this, &AddressWidget::updateGeometries);
		connect(window, &Window::loadingStateChanged, this, &AddressWidget::handleLoadingStateChanged);
		connect(window, &Window::destroyed, this, [&](QObject *object)
		{
			if (qobject_cast<Window*>(object) == m_window)
			{
				setWindow(nullptr);
			}
		});

		if (window->getWebWidget())
		{
			window->getWebWidget()->startWatchingChanges(this, WebWidget::FeedsWatcher);

			connect(window->getWebWidget(), &WebWidget::watchedDataChanged, this, &AddressWidget::handleWatchedDataChanged);
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
	else if (mainWindow && !mainWindow->isAboutToClose() && !m_isUsingSimpleMode)
	{
		connect(this, &AddressWidget::requestedSearch, mainWindow, &MainWindow::search);
	}

	setIcon(window ? window->getIcon() : QIcon());
	setUrl(window ? window->getUrl() : QUrl());
	update();
}

void AddressWidget::setUrl(const QUrl &url, bool force)
{
	const QString text(Utils::isUrlEmpty(url) ? QString() : url.toString());

	if (!m_isUsingSimpleMode)
	{
		updateGeometries();
	}

	if (m_isUsingSimpleMode || ((force || !m_wasEdited || !hasFocus()) && url.scheme() != QLatin1String("javascript")))
	{
		setToolTip(text);
		setText(text);
		setCursorPosition(0);

		m_wasEdited = false;
	}
}

void AddressWidget::setIcon(const QIcon &icon)
{
	if (m_layout.contains(FaviconEntry))
	{
		m_entries[FaviconEntry].icon = (icon.isNull() ? ThemesManager::createIcon((SessionsManager::isPrivate() ? QLatin1String("tab-private") : QLatin1String("tab")), false) : icon);

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
