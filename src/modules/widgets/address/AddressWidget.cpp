/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "../../../ui/BookmarkPropertiesDialog.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ItemViewWidget.h"
#include "../../../ui/LineEditWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"
#include "../../../core/AddressCompletionModel.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/InputInterpreter.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/SearchEnginesManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"

#include <QtCore/QMetaEnum>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionFrame>
#include <QtWidgets/QToolTip>

namespace Otter
{

int AddressWidget::m_entryIdentifierEnumerator(-1);

AddressDelegate::AddressDelegate(ViewMode mode, QObject *parent) : QItemDelegate(parent),
	m_displayMode((SettingsManager::getValue(SettingsManager::AddressField_CompletionDisplayModeOption).toString() == QLatin1String("columns")) ? ColumnsMode : CompactMode),
	m_viewMode(mode)
{
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
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
	const QString description((m_viewMode == HistoryMode) ? Utils::formatDateTime(index.data(HistoryModel::TimeVisitedRole).toDateTime()) : index.data(AddressCompletionModel::TitleRole).toString());
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

void AddressDelegate::optionChanged(int identifier, const QVariant &value)
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

AddressWidget::AddressWidget(Window *window, QWidget *parent) : ComboBoxWidget(parent),
	m_window(nullptr),
	m_lineEdit(new LineEditWidget(this)),
	m_completionModel(new AddressCompletionModel(this)),
	m_completionView(nullptr),
	m_visibleView(nullptr),
	m_clickedEntry(UnknownEntry),
	m_hoveredEntry(UnknownEntry),
	m_completionModes(NoCompletionMode),
	m_hints(WindowsManager::DefaultOpen),
	m_removeModelTimer(0),
	m_isNavigatingCompletion(false),
	m_isUsingSimpleMode(false),
	m_wasPopupVisible(false)
{
	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (!toolBar)
	{
		m_isUsingSimpleMode = true;
	}

	setEditable(true);
	setLineEdit(m_lineEdit);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumWidth(100);
	setItemDelegate(new AddressDelegate(AddressDelegate::HistoryMode, this));
	setInsertPolicy(QComboBox::NoInsert);
	setMouseTracking(true);
	setWindow(window);
	optionChanged(SettingsManager::AddressField_CompletionModeOption, SettingsManager::getValue(SettingsManager::AddressField_CompletionModeOption));
	optionChanged(SettingsManager::AddressField_DropActionOption, SettingsManager::getValue(SettingsManager::AddressField_DropActionOption));
	optionChanged(SettingsManager::AddressField_LayoutOption, SettingsManager::getValue(SettingsManager::AddressField_LayoutOption));
	optionChanged(SettingsManager::AddressField_SelectAllOnFocusOption, SettingsManager::getValue(SettingsManager::AddressField_SelectAllOnFocusOption));

	m_lineEdit->setStyleSheet(QLatin1String("QLineEdit {background:transparent;}"));
	m_lineEdit->installEventFilter(this);

	view()->installEventFilter(this);
	view()->viewport()->setAttribute(Qt::WA_Hover);
	view()->viewport()->setMouseTracking(true);
	view()->viewport()->installEventFilter(this);

	if (toolBar)
	{
		m_lineEdit->setPlaceholderText(tr("Enter address or search…"));

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));

		if (toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
		}
	}

	connect(this, SIGNAL(activated(QString)), this, SLOT(openUrl(QString)));
	connect(m_lineEdit, SIGNAL(textDropped(QString)), this, SLOT(handleUserInput(QString)));
	connect(m_completionModel, SIGNAL(completionReady(QString)), this, SLOT(setCompletion(QString)));
	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateGeometries()));
	connect(HistoryManager::getTypedHistoryModel(), SIGNAL(modelModified()), this, SLOT(updateLineEdit()));
}

void AddressWidget::changeEvent(QEvent *event)
{
	ComboBoxWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			if (!m_isUsingSimpleMode)
			{
				m_lineEdit->setPlaceholderText(tr("Enter address or search…"));
			}

			break;
		case QEvent::LayoutDirectionChange:
			updateGeometries();

			break;
		default:
			break;
	}
}

void AddressWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_removeModelTimer)
	{
		killTimer(m_removeModelTimer);

		m_removeModelTimer = 0;

		const QString text(m_lineEdit->text());

		setModel(new QStandardItemModel(this));
		updateLineEdit();

		m_lineEdit->setText(text);
	}
}

void AddressWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOptionFrame panel;
	panel.initFrom(m_lineEdit);
	panel.rect = rect();
	panel.palette = palette();
	panel.lineWidth = 1;

	style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

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

void AddressWidget::resizeEvent(QResizeEvent *event)
{
	ComboBoxWidget::resizeEvent(event);

	updateGeometries();
}

void AddressWidget::focusInEvent(QFocusEvent *event)
{
	ComboBoxWidget::focusInEvent(event);

	activate(event->reason());
}

void AddressWidget::keyPressEvent(QKeyEvent *event)
{
	ComboBoxWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Down && !view()->isVisible())
	{
		showPopup();
	}
	else if (m_window && event->key() == Qt::Key_Escape)
	{
		const QUrl url(m_window->getUrl());

		if (m_lineEdit->text().trimmed().isEmpty() || m_lineEdit->text().trimmed() != url.toString())
		{
			setText(Utils::isUrlEmpty(url) ? QString() : url.toString());

			if (!m_lineEdit->text().trimmed().isEmpty() && SettingsManager::getValue(SettingsManager::AddressField_SelectAllOnFocusOption).toBool())
			{
				m_lineEdit->selectAll();
			}
		}
		else
		{
			m_window->setFocus();
		}
	}
	else if (!m_isUsingSimpleMode && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
	{
		handleUserInput(m_lineEdit->text().trimmed(), WindowsManager::calculateOpenHints(WindowsManager::CurrentTabOpen, Qt::LeftButton, event->modifiers()));
	}
}

void AddressWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const EntryIdentifier entry(getEntry(event->pos()));
	QMenu menu(this);

	if (entry == UnknownEntry || entry == AddressEntry)
	{
		menu.addAction(tr("Undo"), m_lineEdit, SLOT(undo()), QKeySequence(QKeySequence::Undo))->setEnabled(m_lineEdit->isUndoAvailable());
		menu.addAction(tr("Redo"), m_lineEdit, SLOT(redo()), QKeySequence(QKeySequence::Redo))->setEnabled(m_lineEdit->isRedoAvailable());
		menu.addSeparator();
		menu.addAction(tr("Cut"), m_lineEdit, SLOT(cut()), QKeySequence(QKeySequence::Cut))->setEnabled(m_lineEdit->hasSelectedText());
		menu.addAction(tr("Copy"), m_lineEdit, SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(m_lineEdit->hasSelectedText());
		menu.addAction(tr("Paste"), m_lineEdit, SLOT(paste()), QKeySequence(QKeySequence::Paste))->setEnabled(!QApplication::clipboard()->text().isEmpty());

		if (!m_isUsingSimpleMode)
		{
			menu.addAction(ActionsManager::getAction(ActionsManager::PasteAndGoAction, this));
		}

		menu.addAction(tr("Delete"), m_lineEdit, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(m_lineEdit->hasSelectedText());
		menu.addSeparator();
		menu.addAction(tr("Copy to Note"), m_lineEdit, SLOT(copyToNote()))->setEnabled(!m_lineEdit->text().isEmpty());
		menu.addSeparator();
		menu.addAction(tr("Clear All"), m_lineEdit, SLOT(clear()))->setEnabled(!m_lineEdit->text().isEmpty());
		menu.addAction(tr("Select All"), m_lineEdit, SLOT(selectAll()), QKeySequence(QKeySequence::SelectAll))->setEnabled(!m_lineEdit->text().isEmpty());
	}
	else
	{
		if (entry == WebsiteInformationEntry)
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
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QList<QAction*>(), &menu));
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

	m_wasPopupVisible = (m_popupHideTime.isValid() && m_popupHideTime.msecsTo(QTime::currentTime()) < 100);

	QWidget::mousePressEvent(event);
}

void AddressWidget::mouseMoveEvent(QMouseEvent *event)
{
	const EntryIdentifier entry(getEntry(event->pos()));

	if (entry != m_hoveredEntry)
	{
		if (entry == UnknownEntry || entry == AddressEntry)
		{
			unsetCursor();
		}
		else
		{
			setCursor(Qt::ArrowCursor);
		}

		m_hoveredEntry = entry;
	}

	if (!startDrag(event))
	{
		ComboBoxWidget::mouseMoveEvent(event);
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
					const QList<WebWidget::LinkUrl> feeds((m_window && m_window->getLoadingState() == WindowsManager::FinishedLoadingState) ? m_window->getContentsWidget()->getFeeds() : QList<WebWidget::LinkUrl>());

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

						menu.exec(event->globalPos());
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
							BookmarkPropertiesDialog dialog(url.adjusted(QUrl::RemovePassword), m_window->getTitle(), QString(), nullptr, -1, true, this);
							dialog.exec();
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
				m_popupHideTime = QTime();

				if (m_wasPopupVisible)
				{
					hidePopup();
				}
				else
				{
					showPopup();
				}

				break;
			default:
				break;
		}
	}

	m_clickedEntry = UnknownEntry;

	QWidget::mouseReleaseEvent(event);
}

void AddressWidget::wheelEvent(QWheelEvent *event)
{
	event->ignore();
}

void AddressWidget::showPopup()
{
	if (m_removeModelTimer > 0)
	{
		killTimer(m_removeModelTimer);

		m_removeModelTimer = 0;
	}

	if (view() && view()->isVisible())
	{
		return;
	}

	const QString text(m_lineEdit->text());

	if (model() && model() != HistoryManager::getTypedHistoryModel())
	{
		model()->deleteLater();
	}

	setModel(HistoryManager::getTypedHistoryModel());
	updateLineEdit();

	m_lineEdit->setText(text);

	m_visibleView = view();

	ComboBoxWidget::showPopup();
}

void AddressWidget::hidePopup()
{
	m_popupHideTime = QTime::currentTime();

	ComboBoxWidget::hidePopup();

	QString statusTip;
	QStatusTipEvent statusTipEvent(statusTip);

	QApplication::sendEvent(this, &statusTipEvent);

	m_visibleView = nullptr;

	m_removeModelTimer = startTimer(250);
}

void AddressWidget::hideCompletion()
{
	if (m_completionView)
	{
		m_completionView->hide();
		m_completionView->deleteLater();
		m_completionView = nullptr;

		QString statusTip;
		QStatusTipEvent statusTipEvent(statusTip);

		QApplication::sendEvent(this, &statusTipEvent);

		m_visibleView = nullptr;
	}
}

void AddressWidget::optionChanged(int identifier, const QVariant &value)
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

				disconnect(m_lineEdit, SIGNAL(textEdited(QString)), m_completionModel, SLOT(setFilter(QString)));

				if (m_completionModes != NoCompletionMode)
				{
					connect(m_lineEdit, SIGNAL(textEdited(QString)), m_completionModel, SLOT(setFilter(QString)));
				}
			}

			break;
		case SettingsManager::AddressField_DropActionOption:
			{
				const QString dropAction(value.toString());

				if (dropAction == QLatin1String("pasteAndGo"))
				{
					m_lineEdit->setDropMode(LineEditWidget::ReplaceAndNotifyDropMode);
				}
				else if (dropAction == QLatin1String("replace"))
				{
					m_lineEdit->setDropMode(LineEditWidget::ReplaceDropMode);
				}
				else
				{
					m_lineEdit->setDropMode(LineEditWidget::PasteDropMode);
				}
			}

			break;
		case SettingsManager::AddressField_SelectAllOnFocusOption:
			m_lineEdit->setSelectAllOnFocus(value.toBool());

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
	handleUserInput(url, WindowsManager::CurrentTabOpen);
	updateLineEdit();
}

void AddressWidget::openUrl(const QModelIndex &index)
{
	hideCompletion();

	if (!index.isValid())
	{
		return;
	}

	if (static_cast<AddressCompletionModel::EntryType>(index.data(AddressCompletionModel::TypeRole).toInt()) == AddressCompletionModel::SearchSuggestionType)
	{
		emit requestedSearch(index.data(AddressCompletionModel::TextRole).toString(), QString(), WindowsManager::CurrentTabOpen);
	}
	else
	{
		const QString url(index.data(AddressCompletionModel::UrlRole).toUrl().toString());

		setUrl(url);
		handleUserInput(url, WindowsManager::CurrentTabOpen);
		updateLineEdit();
	}
}

void AddressWidget::removeEntry()
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action)
	{
		QStringList layout(SettingsManager::getValue(SettingsManager::AddressField_LayoutOption).toStringList());
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

void AddressWidget::handleUserInput(const QString &text, WindowsManager::OpenHints hints)
{
	if (hints == WindowsManager::DefaultOpen)
	{
		hints = WindowsManager::calculateOpenHints(WindowsManager::CurrentTabOpen);
	}

	if (!text.isEmpty())
	{
		InputInterpreter *interpreter(new InputInterpreter(this));

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));

		interpreter->interpret(text, hints);
	}
}

void AddressWidget::updateLineEdit()
{
	m_lineEdit->setGeometry(m_lineEditRectangle);
}

void AddressWidget::setCompletion(const QString &filter)
{
	if (filter.isEmpty() || m_completionModel->rowCount() == 0)
	{
		hideCompletion();

		m_lineEdit->setCompletion(QString());
		m_lineEdit->setFocus();

		return;
	}

	if (m_completionModes.testFlag(PopupCompletionMode))
	{
		if (!m_completionView)
		{
			m_completionView = new ItemViewWidget();
			m_completionView->setWindowFlags(Qt::Popup);
			m_completionView->setFocusPolicy(Qt::NoFocus);
			m_completionView->setFocusProxy(m_lineEdit);
			m_completionView->setModel(m_completionModel);
			m_completionView->setItemDelegate(new AddressDelegate(AddressDelegate::CompletionMode, m_completionView));
			m_completionView->setEditTriggers(QAbstractItemView::NoEditTriggers);
			m_completionView->setFixedWidth(width());
			m_completionView->installEventFilter(this);
			m_completionView->header()->setStretchLastSection(true);
			m_completionView->header()->hide();
			m_completionView->viewport()->setAttribute(Qt::WA_Hover);
			m_completionView->viewport()->setMouseTracking(true);
			m_completionView->viewport()->installEventFilter(this);

			connect(m_completionView, SIGNAL(clicked(QModelIndex)), this, SLOT(openUrl(QModelIndex)));
			connect(m_completionView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(setText(QModelIndex)));

			m_completionView->move(mapToGlobal(contentsRect().bottomLeft()));
			m_completionView->show();

			m_visibleView = m_completionView;
		}

		int completionHeight(5);

		if (m_completionModel->rowCount() < 20)
		{
			for (int i = 0; i < m_completionModel->rowCount(); ++i)
			{
				completionHeight += m_completionView->sizeHintForRow(i);
			}
		}
		else
		{
			completionHeight += (20 * m_completionView->sizeHintForRow(0));
		}

		m_completionView->setFixedHeight(completionHeight);
		m_completionView->viewport()->setFixedHeight(completionHeight - 3);
		m_completionView->setCurrentIndex(m_completionModel->index(0, 0));
	}

	if (m_completionModes.testFlag(InlineCompletionMode))
	{
		QString matchedText;

		for (int i = 0; i < m_completionModel->rowCount(); ++i)
		{
			matchedText = m_completionModel->index(i).data(AddressCompletionModel::MatchRole).toString();

			if (!matchedText.isEmpty())
			{
				m_lineEdit->setCompletion(matchedText);

				break;
			}
		}
	}
}

void AddressWidget::activate(Qt::FocusReason reason)
{
	m_lineEdit->activate(reason);
}

void AddressWidget::updateGeometries()
{
	QHash<EntryIdentifier, EntryDefinition> entries;
	QList<EntryDefinition> leadingEntries;
	QList<EntryDefinition> trailingEntries;
	QMargins margins(qMax(((height() - 16) / 2), 2), 0, 2, 0);
	bool isLeading(true);
	bool isRightToLeft(layoutDirection() == Qt::RightToLeft);

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
					const WindowsManager::ContentStates state(m_window ? m_window->getContentState() : WindowsManager::UnknownContentState);

					if (state.testFlag(WindowsManager::FraudContentState))
					{
						icon = QLatin1String("badge-fraud");
					}
					else if (state.testFlag(WindowsManager::SecureContentState))
					{
						icon = QLatin1String("badge-secure");
					}
					else if (state.testFlag(WindowsManager::RemoteContentState))
					{
						icon = QLatin1String("badge-remote");
					}
					else if (state.testFlag(WindowsManager::LocalContentState))
					{
						icon = QLatin1String("badge-local");
					}
					else if (state.testFlag(WindowsManager::ApplicationContentState))
					{
						icon = QLatin1String("otter-browser");
					}

					definition.title = QT_TR_NOOP("Show website information");
					definition.icon = ThemesManager::getIcon(icon, false);
				}

				break;
			case FaviconEntry:
				definition.icon = (m_window ? m_window->getIcon() : ThemesManager::getIcon((SessionsManager::isPrivate() ? QLatin1String("tab-private") : QLatin1String("tab")), false));

				break;
			case ListFeedsEntry:
				if (!m_window || m_window->isAboutToClose() || m_window->getLoadingState() != WindowsManager::FinishedLoadingState || m_window->getContentsWidget()->getFeeds().isEmpty())
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
					if (!m_window || m_window->isAboutToClose() || m_window->getLoadingState() != WindowsManager::FinishedLoadingState)
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

					if (!m_window || m_window->isAboutToClose() || m_window->getLoadingState() != WindowsManager::FinishedLoadingState || Utils::isUrlEmpty(url) || url.scheme() == QLatin1String("about") || !PasswordsManager::hasPasswords(url, PasswordsManager::FormPassword))
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

	if (margins.left() > 0)
	{
		margins.setLeft(margins.left() - 2);
	}

	if (margins.right() > 0)
	{
		margins.setRight(margins.right() - 2);
	}

	m_lineEdit->resize((width() - margins.left() - margins.right()), height());
	m_lineEdit->move(margins.left(), 0);

	m_lineEditRectangle = m_lineEdit->geometry();
}

void AddressWidget::setWindow(Window *window)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (m_window && !m_window->isAboutToClose() && (!sender() || sender() != m_window))
	{
		m_window->detachAddressWidget(this);

		disconnect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), m_window.data(), SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), m_window.data(), SLOT(handleSearchRequest(QString,QString,WindowsManager::OpenHints)));
		disconnect(m_window.data(), SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
		disconnect(m_window.data(), SIGNAL(urlChanged(QUrl,bool)), this, SLOT(setUrl(QUrl,bool)));
		disconnect(m_window.data(), SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		disconnect(m_window.data(), SIGNAL(contentStateChanged(WindowsManager::ContentStates)), this, SLOT(updateGeometries()));
		disconnect(m_window.data(), SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(updateGeometries()));
	}

	m_window = window;

	if (window)
	{
		if (mainWindow)
		{
			disconnect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(QUrl,WindowsManager::OpenHints)));
			disconnect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(BookmarksItem*,WindowsManager::OpenHints)));
			disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
		}

		window->attachAddressWidget(this);

		connect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), window, SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), window, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), window, SLOT(handleSearchRequest(QString,QString,WindowsManager::OpenHints)));
		connect(window, SIGNAL(urlChanged(QUrl,bool)), this, SLOT(setUrl(QUrl,bool)));
		connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		connect(window, SIGNAL(contentStateChanged(WindowsManager::ContentStates)), this, SLOT(updateGeometries()));
		connect(window, SIGNAL(loadingStateChanged(WindowsManager::LoadingState)), this, SLOT(updateGeometries()));

		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));

		if (!toolBar || toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(window, SIGNAL(aboutToClose()), this, SLOT(setWindow()));
		}
	}
	else if (mainWindow && !mainWindow->isAboutToClose() && !m_isUsingSimpleMode)
	{
		connect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(QUrl,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(BookmarksItem*,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
	}

	setIcon(window ? window->getIcon() : QIcon());
	setUrl(window ? window->getUrl() : QUrl());
	update();
}

void AddressWidget::setText(const QString &text)
{
	m_lineEdit->setText(text);
}

void AddressWidget::setText(const QModelIndex &index)
{
	if (m_isNavigatingCompletion)
	{
		m_isNavigatingCompletion = false;

		m_lineEdit->setText(index.data(AddressCompletionModel::TextRole).toString());
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

		m_lineEdit->setText(text);
		m_lineEdit->setCursorPosition(0);
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

QString AddressWidget::getText() const
{
	return m_lineEdit->text();
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

	return ComboBoxWidget::event(event);
}

bool AddressWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_lineEdit && event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && mouseEvent->button() == Qt::MiddleButton && m_lineEdit->text().isEmpty() && !QApplication::clipboard()->text().isEmpty() && SettingsManager::getValue(SettingsManager::AddressField_PasteAndGoOnMiddleClickOption).toBool())
		{
			handleUserInput(QApplication::clipboard()->text().trimmed(), WindowsManager::CurrentTabOpen);

			event->accept();

			return true;
		}
	}
	else if (object == m_lineEdit && event->type() == QEvent::DragEnter)
	{
		QDragEnterEvent *dragEnterEvent(static_cast<QDragEnterEvent*>(event));

		if (dragEnterEvent && dragEnterEvent->mimeData()->hasUrls())
		{
			event->accept();
		}
	}
	else if (object == m_completionView && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent)
		{
			m_lineEdit->event(event);

			if (event->isAccepted())
			{
				if (!m_lineEdit->hasFocus())
				{
					hideCompletion();
				}

				return true;
			}

			switch (keyEvent->key())
			{
				case Qt::Key_Up:
				case Qt::Key_Down:
				case Qt::Key_PageUp:
				case Qt::Key_PageDown:
				case Qt::Key_End:
				case Qt::Key_Home:
					m_isNavigatingCompletion = true;

					return false;
				case Qt::Key_Enter:
				case Qt::Key_Return:
				case Qt::Key_Tab:
				case Qt::Key_Backtab:
				case Qt::Key_Escape:
				case Qt::Key_F4:
					if (keyEvent->key() == Qt::Key_F4 && !keyEvent->modifiers().testFlag(Qt::AltModifier))
					{
						break;
					}

					hideCompletion();

					m_lineEdit->setFocus();

					if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
					{
						openUrl(m_lineEdit->text());
					}

					break;
				default:
					break;
			}

			return true;
		}
	}
	else if (object == m_completionView && event->type() == QEvent::MouseButtonPress && !m_completionView->viewport()->underMouse())
	{
		hideCompletion();

		m_lineEdit->setFocus();

		return true;
	}
	else if (object == m_completionView && (event->type() == QEvent::InputMethod || event->type() == QEvent::ShortcutOverride))
	{
		QApplication::sendEvent(m_lineEdit, event);
	}
	else if (m_visibleView && object == m_visibleView->viewport() && event->type() == QEvent::MouseMove)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent)
		{
			const QModelIndex index(m_visibleView->indexAt(mouseEvent->pos()));

			if (index.isValid() && m_visibleView == m_completionView)
			{
				m_completionView->setCurrentIndex(index);
			}

			QStatusTipEvent statusTipEvent(index.data(Qt::StatusTipRole).toString());

			QApplication::sendEvent(this, &statusTipEvent);
		}
	}
	else if (object == m_visibleView && (event->type() == QEvent::Close || event->type() == QEvent::Hide || event->type() == QEvent::Leave))
	{
		QString statusTip;
		QStatusTipEvent statusTipEvent(statusTip);

		QApplication::sendEvent(this, &statusTipEvent);
	}

	return QObject::eventFilter(object, event);
}

}
