/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AddressWidget.h"
#include "../AddressDelegate.h"
#include "../BookmarkPropertiesDialog.h"
#include "../ContentsWidget.h"
#include "../LineEditWidget.h"
#include "../MainWindow.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/AddressCompletionModel.h"
#include "../../core/BookmarksManager.h"
#include "../../core/InputInterpreter.h"
#include "../../core/HistoryManager.h"
#include "../../core/SearchesManager.h"
#include "../../core/Utils.h"

#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDrag>
#include <QtGui/QPainter>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionFrame>
#include <QtWidgets/QToolTip>

namespace Otter
{

AddressWidget::AddressWidget(Window *window, QWidget *parent) : QComboBox(parent),
	m_window(NULL),
	m_lineEdit(new LineEditWidget(this)),
	m_completionModel(new AddressCompletionModel(this)),
	m_completionView(NULL),
	m_bookmarkLabel(NULL),
	m_feedsLabel(NULL),
	m_loadPluginsLabel(NULL),
	m_urlIconLabel(NULL),
	m_completionModes(NoCompletionMode),
	m_hints(WindowsManager::DefaultOpen),
	m_removeModelTimer(0),
	m_isHistoryDropdownEnabled(SettingsManager::getValue(QLatin1String("AddressField/EnableHistoryDropdown")).toBool()),
	m_isUsingSimpleMode(false),
	m_wasPopupVisible(false)
{
	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (!toolBar)
	{
		m_isUsingSimpleMode = true;
	}

	setEditable(true);
	setLineEdit(m_lineEdit);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumWidth(100);
	setItemDelegate(new AddressDelegate(true, this));
	setInsertPolicy(QComboBox::NoInsert);
	setMouseTracking(true);
	setWindow(window);
	optionChanged(QLatin1String("AddressField/CompletionMode"), SettingsManager::getValue(QLatin1String("AddressField/CompletionMode")));
	optionChanged(QLatin1String("AddressField/DropAction"), SettingsManager::getValue(QLatin1String("AddressField/DropAction")));
	optionChanged(QLatin1String("AddressField/SelectAllOnFocus"), SettingsManager::getValue(QLatin1String("AddressField/SelectAllOnFocus")));

	m_lineEdit->setStyleSheet(QLatin1String("QLineEdit {background:transparent;}"));
	m_lineEdit->installEventFilter(this);

	if (toolBar)
	{
		optionChanged(QLatin1String("AddressField/ShowBookmarkIcon"), SettingsManager::getValue(QLatin1String("AddressField/ShowBookmarkIcon")));
		optionChanged(QLatin1String("AddressField/ShowUrlIcon"), SettingsManager::getValue(QLatin1String("AddressField/ShowUrlIcon")));

		m_lineEdit->setPlaceholderText(tr("Enter address or search…"));

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));

		if (toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
		}
	}

	connect(this, SIGNAL(activated(QString)), this, SLOT(openUrl(QString)));
	connect(m_lineEdit, SIGNAL(textDropped(QString)), this, SLOT(handleUserInput(QString)));
	connect(m_completionModel, SIGNAL(completionReady(QString)), this, SLOT(setCompletion(QString)));
	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmark()));
	connect(HistoryManager::getInstance(), SIGNAL(typedHistoryModelModified()), this, SLOT(updateLineEdit()));
}

void AddressWidget::changeEvent(QEvent *event)
{
	QComboBox::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && !m_isUsingSimpleMode)
	{
		m_lineEdit->setPlaceholderText(tr("Enter address or search…"));
	}
}

void AddressWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_removeModelTimer)
	{
		killTimer(m_removeModelTimer);

		m_removeModelTimer = 0;

		const QString text = m_lineEdit->text();

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

	if (m_isHistoryDropdownEnabled || m_isUsingSimpleMode)
	{
		QStyleOption arrow;
		arrow.initFrom(this);
		arrow.rect = m_historyDropdownArrowRectangle;

		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, this);
	}

	if (m_isUsingSimpleMode)
	{
		return;
	}

	const WindowsManager::ContentStates state = (m_window ? m_window->getContentState() : WindowsManager::UnknownContentState);
	QString badgeIcon = QLatin1String("unknown");
	QColor badgeColor = QColor(245, 245, 245);

	if (state.testFlag(WindowsManager::FraudContentState))
	{
		badgeIcon = QLatin1String("badge-fraud");
		badgeColor = QColor(233, 111, 125);
	}
	else if (state.testFlag(WindowsManager::TrustedContentState))
	{
		badgeIcon = QLatin1String("badge-trusted");
		badgeColor = QColor(173, 232, 153);
	}
	else if (state.testFlag(WindowsManager::SecureContentState))
	{
		badgeIcon = QLatin1String("badge-secure");
		badgeColor = QColor(245, 222, 160);
	}
	else if (state.testFlag(WindowsManager::RemoteContentState))
	{
		badgeIcon = QLatin1String("badge-remote");
	}
	else if (state.testFlag(WindowsManager::LocalContentState))
	{
		badgeIcon = QLatin1String("badge-local");
	}
	else if (state.testFlag(WindowsManager::ApplicationContentState))
	{
		badgeIcon = QLatin1String("otter-browser");
	}

	panel.palette.setColor(QPalette::Base, badgeColor);
	panel.state = QStyle::State_Active;

	QRect rectangle = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	rectangle.setWidth(30);
	rectangle.moveTo(panel.lineWidth, panel.lineWidth);

	m_securityBadgeRectangle = rectangle;

	painter.fillRect(rectangle, badgeColor);
	painter.setClipRect(rectangle);

	style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

	QPalette linePalette = palette();
	linePalette.setCurrentColorGroup(QPalette::Disabled);

	painter.setPen(QPen(linePalette.mid().color(), 1));
	painter.drawLine(rectangle.right(), rectangle.top(), rectangle.right(), rectangle.bottom());

	if (!badgeIcon.isEmpty())
	{
		Utils::getIcon(badgeIcon, false).paint(&painter, rectangle.adjusted(4, 4, -4, -4));
	}
}

void AddressWidget::resizeEvent(QResizeEvent *event)
{
	QComboBox::resizeEvent(event);

	updateLineEdit();

	if (m_isHistoryDropdownEnabled || m_isUsingSimpleMode)
	{
		QStyleOptionFrame panel;
		panel.initFrom(m_lineEdit);
		panel.rect = rect();
		panel.lineWidth = 1;

		m_historyDropdownArrowRectangle = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
		m_historyDropdownArrowRectangle.setLeft(m_historyDropdownArrowRectangle.left() + m_historyDropdownArrowRectangle.width() - 12);
	}

	updateIcons();
}

void AddressWidget::focusInEvent(QFocusEvent *event)
{
	if (event->reason() == Qt::MouseFocusReason && m_lineEdit->childAt(mapFromGlobal(QCursor::pos())))
	{
		return;
	}

	QComboBox::focusInEvent(event);

	activate(event->reason());
}

void AddressWidget::keyPressEvent(QKeyEvent *event)
{
	QComboBox::keyPressEvent(event);

	if (event->key() == Qt::Key_Down && !view()->isVisible())
	{
		showPopup();
	}
	else if (m_window && event->key() == Qt::Key_Escape)
	{
		const QUrl url = m_window->getUrl();

		if (m_lineEdit->text().trimmed().isEmpty() || m_lineEdit->text().trimmed() != url.toString())
		{
			setText(Utils::isUrlEmpty(url) ? QString() : url.toString());

			if (!m_lineEdit->text().trimmed().isEmpty() && SettingsManager::getValue(QLatin1String("AddressField/SelectAllOnFocus")).toBool())
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
		handleUserInput(m_lineEdit->text().trimmed(), WindowsManager::calculateOpenHints(event->modifiers(), Qt::LeftButton, WindowsManager::CurrentTabOpen));
	}
}

void AddressWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);

	if (m_securityBadgeRectangle.contains(event->pos()))
	{
		if (m_window)
		{
			menu.addAction(m_window->getContentsWidget()->getAction(ActionsManager::WebsiteInformationAction));
		}
		else
		{
			Action *websiteInformationAction = new Action(ActionsManager::WebsiteInformationAction);
			websiteInformationAction->setEnabled(false);

			menu.addAction(websiteInformationAction);
		}
	}
	else
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

		menu.addAction(tr("Delete"), this, SLOT(deleteText()), QKeySequence(QKeySequence::Delete))->setEnabled(m_lineEdit->hasSelectedText());
		menu.addSeparator();
		menu.addAction(tr("Copy to Note"), this, SLOT(copyToNote()))->setEnabled(!m_lineEdit->text().isEmpty());
		menu.addSeparator();
		menu.addAction(tr("Clear All"), m_lineEdit, SLOT(clear()))->setEnabled(!m_lineEdit->text().isEmpty());
		menu.addAction(tr("Select All"), m_lineEdit, SLOT(selectAll()))->setEnabled(!m_lineEdit->text().isEmpty());
	}

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parentWidget());

	if (toolBar)
	{
		menu.addSeparator();
		menu.addMenu(ToolBarWidget::createCustomizationMenu(toolBar->getIdentifier(), QList<QAction*>(), &menu));
	}

	menu.exec(event->globalPos());
}

void AddressWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && m_securityBadgeRectangle.contains(event->pos()))
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
	if ((!m_isUsingSimpleMode && m_securityBadgeRectangle.contains(event->pos())) || ((m_isHistoryDropdownEnabled || m_isUsingSimpleMode) && m_historyDropdownArrowRectangle.contains(event->pos())))
	{
		setCursor(Qt::ArrowCursor);
	}
	else
	{
		setCursor(Qt::IBeamCursor);
	}

	if (!startDrag(event))
	{
		QComboBox::mouseMoveEvent(event);
	}
}

void AddressWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_securityBadgeRectangle.contains(event->pos()) && m_window)
	{
		m_window->triggerAction(ActionsManager::WebsiteInformationAction);

		event->accept();

		return;
	}

	if (m_historyDropdownArrowRectangle.contains(event->pos()))
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

	const QString text = m_lineEdit->text();

	if (model() && model() != HistoryManager::getTypedHistoryModel())
	{
		model()->deleteLater();
	}

	setModel(HistoryManager::getTypedHistoryModel());
	updateLineEdit();

	m_lineEdit->setText(text);

	QComboBox::showPopup();
}

void AddressWidget::hidePopup()
{
	m_popupHideTime = QTime::currentTime();

	QComboBox::hidePopup();

	m_removeModelTimer = startTimer(250);
}

void AddressWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("AddressField/CompletionMode"))
	{
		const QString completionMode = value.toString();

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
	else if (option == QLatin1String("AddressField/DropAction"))
	{
		const QString dropAction = value.toString();

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
	else if (option == QLatin1String("AddressField/SelectAllOnFocus"))
	{
		m_lineEdit->setSelectAllOnFocus(value.toBool());
	}
	else if (option == QLatin1String("AddressField/EnableHistoryDropdown"))
	{
		m_isHistoryDropdownEnabled = value.toBool();

		if (m_isHistoryDropdownEnabled || m_isUsingSimpleMode)
		{
			QStyleOptionFrame panel;
			panel.initFrom(m_lineEdit);
			panel.rect = rect();
			panel.lineWidth = 1;

			m_historyDropdownArrowRectangle = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
			m_historyDropdownArrowRectangle.setLeft(m_historyDropdownArrowRectangle.left() + m_historyDropdownArrowRectangle.width() - 12);
		}

		updateIcons();
	}
	else if (option == QLatin1String("AddressField/ShowBookmarkIcon"))
	{
		if (value.toBool() && !m_bookmarkLabel)
		{
			m_bookmarkLabel = new QLabel(this);
			m_bookmarkLabel->setObjectName(QLatin1String("Bookmark"));
			m_bookmarkLabel->setAutoFillBackground(false);
			m_bookmarkLabel->setFixedSize(16, 16);
			m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
			m_bookmarkLabel->setCursor(Qt::ArrowCursor);
			m_bookmarkLabel->setFocusPolicy(Qt::NoFocus);
			m_bookmarkLabel->installEventFilter(this);

			updateIcons();
		}
		else if (!value.toBool() && m_bookmarkLabel)
		{
			m_bookmarkLabel->deleteLater();
			m_bookmarkLabel = NULL;

			updateIcons();
		}
	}
	else if (option == QLatin1String("AddressField/ShowUrlIcon"))
	{
		if (value.toBool() && !m_urlIconLabel)
		{
			m_urlIconLabel = new QLabel(this);
			m_urlIconLabel->setObjectName(QLatin1String("Url"));
			m_urlIconLabel->setAutoFillBackground(false);
			m_urlIconLabel->setFixedSize(16, 16);
			m_urlIconLabel->setPixmap((m_window ? m_window->getIcon() : Utils::getIcon(QLatin1String("tab"))).pixmap(m_urlIconLabel->size()));
			m_urlIconLabel->setCursor(Qt::ArrowCursor);
			m_urlIconLabel->setFocusPolicy(Qt::NoFocus);
			m_urlIconLabel->installEventFilter(this);

			if (m_window)
			{
				connect(m_window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
			}
		}
		else
		{
			if (!value.toBool() && m_urlIconLabel)
			{
				m_urlIconLabel->deleteLater();
				m_urlIconLabel = NULL;

				updateIcons();
			}

			if (m_window)
			{
				disconnect(m_window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
			}
		}

		updateIcons();
	}
	else if (option == QLatin1String("AddressField/ShowLoadPluginsIcon") && m_window)
	{
		if (value.toBool())
		{
			connect(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}
		else
		{
			disconnect(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}

		updateLoadPlugins();
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
	if (m_completionView)
	{
		m_completionView->hide();
		m_completionView->deleteLater();
		m_completionView = NULL;
	}

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

void AddressWidget::removeIcon()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		SettingsManager::setValue(QStringLiteral("AddressField/Show%1Icon").arg(action->data().toString()), false);
	}
}

void AddressWidget::handleUserInput(const QString &text, WindowsManager::OpenHints hints)
{
	if (hints == WindowsManager::DefaultOpen)
	{
		hints = WindowsManager::calculateOpenHints(QApplication::keyboardModifiers(), Qt::LeftButton, WindowsManager::CurrentTabOpen);
	}

	if (!text.isEmpty())
	{
		InputInterpreter *interpreter = new InputInterpreter(this);

		connect(interpreter, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)));
		connect(interpreter, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)));

		interpreter->interpret(text, hints);
	}
}

void AddressWidget::updateBookmark()
{
	if (!m_bookmarkLabel)
	{
		return;
	}

	const QUrl url = getUrl();

	if (Utils::isUrlEmpty(url) || url.scheme() == QLatin1String("about"))
	{
		m_bookmarkLabel->setEnabled(false);
		m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
		m_bookmarkLabel->setToolTip(QString());

		return;
	}

	const bool hasBookmark = BookmarksManager::hasBookmark(url);

	m_bookmarkLabel->setEnabled(true);
	m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), (hasBookmark ? QIcon::Active : QIcon::Disabled)));
	m_bookmarkLabel->setToolTip(hasBookmark ? tr("Remove Bookmark") : tr("Add Bookmark"));
}

void AddressWidget::updateFeeds()
{
	const QList<LinkUrl> feeds = ((m_window && m_window->getLoadingState() == LoadedState) ? m_window->getContentsWidget()->getFeeds() : QList<LinkUrl>());

	if (!feeds.isEmpty() && !m_feedsLabel)
	{
		m_feedsLabel = new QLabel(this);
		m_feedsLabel->show();
		m_feedsLabel->setObjectName(QLatin1String("Feeds"));
		m_feedsLabel->setAutoFillBackground(false);
		m_feedsLabel->setFixedSize(16, 16);
		m_feedsLabel->setPixmap(Utils::getIcon(QLatin1String("application-rss+xml")).pixmap(m_feedsLabel->size()));
		m_feedsLabel->setCursor(Qt::ArrowCursor);
		m_feedsLabel->setToolTip(tr("Feed List"));
		m_feedsLabel->setCursor(Qt::ArrowCursor);
		m_feedsLabel->setFocusPolicy(Qt::NoFocus);
		m_feedsLabel->installEventFilter(this);

		updateIcons();
	}
	else if (feeds.isEmpty() && m_feedsLabel)
	{
		m_feedsLabel->deleteLater();
		m_feedsLabel = NULL;

		updateIcons();
	}
}

void AddressWidget::updateLoadPlugins()
{
	const bool canLoadPlugins = (SettingsManager::getValue(QLatin1String("AddressField/ShowLoadPluginsIcon")).toBool() && m_window && !m_window->isAboutToClose() && m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction) && m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction)->isEnabled());

	if (canLoadPlugins && !m_loadPluginsLabel)
	{
		m_loadPluginsLabel = new QLabel(this);
		m_loadPluginsLabel->show();
		m_loadPluginsLabel->setObjectName(QLatin1String("LoadPlugins"));
		m_loadPluginsLabel->setAutoFillBackground(false);
		m_loadPluginsLabel->setFixedSize(16, 16);
		m_loadPluginsLabel->setPixmap(Utils::getIcon(QLatin1String("preferences-plugin")).pixmap(m_loadPluginsLabel->size()));
		m_loadPluginsLabel->setCursor(Qt::ArrowCursor);
		m_loadPluginsLabel->setToolTip(tr("Click to load all plugins on the page"));
		m_loadPluginsLabel->setCursor(Qt::ArrowCursor);
		m_loadPluginsLabel->setFocusPolicy(Qt::NoFocus);
		m_loadPluginsLabel->installEventFilter(this);

		updateIcons();
	}
	else if (!canLoadPlugins && m_loadPluginsLabel)
	{
		m_loadPluginsLabel->deleteLater();
		m_loadPluginsLabel = NULL;

		updateIcons();
	}
}

void AddressWidget::updateLineEdit()
{
	m_lineEdit->setGeometry(m_lineEditRectangle);
}

void AddressWidget::updateIcons()
{
	QMargins margins(5, 0, 0, 0);

	if (!m_isUsingSimpleMode)
	{
		margins.setLeft(margins.left() + 30);
	}

	if (m_urlIconLabel)
	{
		m_urlIconLabel->move(36, ((height() - m_urlIconLabel->height()) / 2));

		margins.setLeft(margins.left() + 22);
	}

	if (m_isHistoryDropdownEnabled || m_isUsingSimpleMode)
	{
		margins.setRight(margins.right() + m_historyDropdownArrowRectangle.width());
	}

	if (m_bookmarkLabel)
	{
		margins.setRight(margins.right() + 22);

		m_bookmarkLabel->move((width() - margins.right()), ((height() - m_bookmarkLabel->height()) / 2));
	}

	if (m_feedsLabel)
	{
		margins.setRight(margins.right() + 22);

		m_feedsLabel->move((width() - margins.right()), ((height() - m_feedsLabel->height()) / 2));
	}

	if (m_loadPluginsLabel)
	{
		margins.setRight(margins.right() + 22);

		m_loadPluginsLabel->move((width() - margins.right()), ((height() - m_loadPluginsLabel->height()) / 2));
	}

	margins.setRight(margins.right() + 3);

	m_lineEdit->resize((width() - margins.left() - margins.right()), height());
	m_lineEdit->move(QPoint(margins.left(), 0));

	m_lineEditRectangle = m_lineEdit->geometry();
}

void AddressWidget::setCompletion(const QString &filter)
{
	if (filter.isEmpty() || m_completionModel->rowCount() == 0)
	{
		if (m_completionView)
		{
			m_completionView->hide();
			m_completionView->deleteLater();
			m_completionView = NULL;
		}

		m_lineEdit->setCompletion(QString());
		m_lineEdit->setFocus();

		return;
	}

	if (m_completionModes.testFlag(PopupCompletionMode))
	{
		if (!m_completionView)
		{
			m_completionView = new QListView();
			m_completionView->setWindowFlags(Qt::Popup);
			m_completionView->setFocusPolicy(Qt::NoFocus);
			m_completionView->setFocusProxy(m_lineEdit);
			m_completionView->setModel(m_completionModel);
			m_completionView->setItemDelegate(new AddressDelegate(true, this));
			m_completionView->setEditTriggers(QAbstractItemView::NoEditTriggers);
			m_completionView->viewport()->setAttribute(Qt::WA_Hover);
			m_completionView->viewport()->setMouseTracking(true);
			m_completionView->setFixedWidth(width());
			m_completionView->installEventFilter(this);
			m_completionView->viewport()->installEventFilter(this);

			connect(m_completionView, SIGNAL(clicked(QModelIndex)), this, SLOT(openUrl(QModelIndex)));
			connect(m_completionView, SIGNAL(entered(QModelIndex)), this, SLOT(setText(QModelIndex)));

			m_completionView->move(mapToGlobal(contentsRect().bottomLeft()));
			m_completionView->show();
		}

		m_completionView->setFixedHeight((qMin(m_completionModel->rowCount(), 10) * m_completionView->sizeHintForRow(0)) + 3);
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

void AddressWidget::setIcon(const QIcon &icon)
{
	if (m_urlIconLabel)
	{
		m_urlIconLabel->setPixmap((icon.isNull() ? Utils::getIcon(QLatin1String("tab")) : icon).pixmap(m_urlIconLabel->size()));
	}
}

void AddressWidget::setText(const QString &text)
{
	m_lineEdit->setText(text);
}

void AddressWidget::setText(const QModelIndex &index)
{
	m_lineEdit->setText(index.data(AddressCompletionModel::TextRole).toString());
}

void AddressWidget::setUrl(const QUrl &url, bool force)
{
	if (!m_isUsingSimpleMode)
	{
		updateBookmark();
		updateFeeds();
	}

	if ((force || !hasFocus()) && url.scheme() != QLatin1String("javascript"))
	{
		const QString text(Utils::isUrlEmpty(url) ? QString() : url.toString());

		setToolTip(text);

		m_lineEdit->setText(text);
		m_lineEdit->setCursorPosition(0);
	}
}

void AddressWidget::setWindow(Window *window)
{
	MainWindow *mainWindow = MainWindow::findMainWindow(this);

	if (m_window && (!sender() || sender() != m_window) && !m_window->isAboutToClose())
	{
		m_window->detachAddressWidget(this);

		disconnect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), m_window.data(), SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), m_window.data(), SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
		disconnect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), m_window.data(), SLOT(handleSearchRequest(QString,QString,WindowsManager::OpenHints)));
		disconnect(m_window.data(), SIGNAL(destroyed(QObject*)), this, SLOT(setWindow()));
		disconnect(m_window.data(), SIGNAL(urlChanged(QUrl,bool)), this, SLOT(setUrl(QUrl,bool)));
		disconnect(m_window.data(), SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		disconnect(m_window.data(), SIGNAL(loadingStateChanged(WindowLoadingState)), this, SLOT(updateFeeds()));
		disconnect(m_window.data(), SIGNAL(contentStateChanged(WindowsManager::ContentStates)), this, SLOT(update()));

		if (m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction))
		{
			disconnect(m_window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}
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

		if (m_urlIconLabel)
		{
			connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
		}

		connect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), window, SLOT(handleOpenUrlRequest(QUrl,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), window, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), window, SLOT(handleSearchRequest(QString,QString,WindowsManager::OpenHints)));
		connect(window, SIGNAL(urlChanged(QUrl,bool)), this, SLOT(setUrl(QUrl,bool)));
		connect(window, SIGNAL(loadingStateChanged(WindowLoadingState)), this, SLOT(updateFeeds()));
		connect(window, SIGNAL(contentStateChanged(WindowsManager::ContentStates)), this, SLOT(update()));

		ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parentWidget());

		if (!toolBar || toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
		{
			connect(window, SIGNAL(aboutToClose()), this, SLOT(setWindow()));
		}

		if (window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction))
		{
			connect(window->getContentsWidget()->getAction(ActionsManager::LoadPluginsAction), SIGNAL(changed()), this, SLOT(updateLoadPlugins()));
		}
	}
	else if (mainWindow && !m_isUsingSimpleMode)
	{
		connect(this, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(QUrl,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(BookmarksItem*,WindowsManager::OpenHints)));
		connect(this, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
	}

	setIcon(window ? window->getIcon() : QIcon());
	setUrl(window ? window->getUrl() : QUrl());
	updateLoadPlugins();
	update();
}

QString AddressWidget::getText() const
{
	return m_lineEdit->text();
}

QUrl AddressWidget::getUrl() const
{
	return (m_window ? m_window->getUrl() : QUrl(QLatin1String("about:blank")));
}

bool AddressWidget::startDrag(QMouseEvent *event)
{
	if (!event->buttons().testFlag(Qt::LeftButton) || m_dragStartPosition.isNull() || (event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
	{
		return false;
	}

	QList<QUrl> urls;
	urls << getUrl();

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData();
	mimeData->setText(getUrl().toString());
	mimeData->setUrls(urls);

	drag->setMimeData(mimeData);
	drag->setPixmap((m_window ? m_window->getIcon() : Utils::getIcon(QLatin1String("tab"))).pixmap(16, 16));
	drag->exec(Qt::CopyAction);

	return true;
}

bool AddressWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);

		if (helpEvent && m_securityBadgeRectangle.contains(helpEvent->pos()))
		{
			QToolTip::showText(helpEvent->globalPos(), tr("Show Website Information"));

			return true;
		}
	}

	return QComboBox::event(event);
}

bool AddressWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_lineEdit && event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::MiddleButton && m_lineEdit->text().isEmpty() && !QApplication::clipboard()->text().isEmpty() && SettingsManager::getValue(QLatin1String("AddressField/PasteAndGoOnMiddleClick")).toBool())
		{
			handleUserInput(QApplication::clipboard()->text().trimmed(), WindowsManager::CurrentTabOpen);

			event->accept();

			return true;
		}
	}
	else if (object == m_urlIconLabel && m_urlIconLabel && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent->button() == Qt::LeftButton && m_securityBadgeRectangle.contains(mouseEvent->pos()))
		{
			m_dragStartPosition = mouseEvent->pos();
		}
		else
		{
			m_dragStartPosition = QPoint();
		}

		mouseEvent->accept();

		return true;
	}
	else if (object == m_urlIconLabel && m_urlIconLabel && event->type() == QEvent::MouseMove)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && startDrag(mouseEvent))
		{
			return true;
		}
	}
	else if (object == m_bookmarkLabel && m_bookmarkLabel && m_window && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			if (m_bookmarkLabel->isEnabled())
			{
				const QUrl url = getUrl();

				if (BookmarksManager::hasBookmark(url))
				{
					BookmarksManager::removeBookmark(url);
				}
				else
				{
					BookmarkPropertiesDialog dialog(url.adjusted(QUrl::RemovePassword), m_window->getTitle(), QString(), NULL, -1, true, this);
					dialog.exec();
				}

				updateBookmark();
			}

			event->accept();

			return true;
		}
	}
	else if (object == m_feedsLabel && m_feedsLabel && m_window && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			const QList<LinkUrl> feeds = ((m_window && m_window->getLoadingState() == LoadedState) ? m_window->getContentsWidget()->getFeeds() : QList<LinkUrl>());

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

				menu.exec(mouseEvent->globalPos());
			}

			event->accept();

			return true;
		}
	}
	else if (object == m_loadPluginsLabel && m_loadPluginsLabel && m_window && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			m_window->getContentsWidget()->triggerAction(ActionsManager::LoadPluginsAction);

			event->accept();

			return true;
		}
	}
	else if (object != m_lineEdit && event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

		if (contextMenuEvent)
		{
			QMenu menu(this);
			QAction *action = menu.addAction(tr("Remove This Icon"), this, SLOT(removeIcon()));
			action->setData(object->objectName());

			menu.exec(contextMenuEvent->globalPos());

			event->accept();

			return true;
		}
	}
	else if (object == m_completionView && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent)
		{
			m_lineEdit->event(event);

			if (event->isAccepted())
			{
				if (!m_lineEdit->hasFocus())
				{
					m_completionView->hide();
					m_completionView->deleteLater();
					m_completionView = NULL;
				}

				return true;
			}

			switch (keyEvent->key())
			{
				case Qt::Key_End:
				case Qt::Key_Home:
				case Qt::Key_Up:
					if (keyEvent->key() == Qt::Key_Up && m_completionModel->rowCount() > 1)
					{
						const QModelIndex index((m_completionView->currentIndex().row() == 0) ? m_completionModel->index(m_completionModel->rowCount() - 1) : m_completionModel->index(m_completionView->currentIndex().row() - 1));

						m_completionView->setCurrentIndex(index);

						setText(index);

						return true;
					}
				case Qt::Key_Down:
					if (keyEvent->key() == Qt::Key_Down && m_completionModel->rowCount() > 1)
					{
						const QModelIndex index((m_completionView->currentIndex().row() == (m_completionModel->rowCount() - 1)) ? m_completionModel->index(0) : m_completionModel->index(m_completionView->currentIndex().row() + 1));

						m_completionView->setCurrentIndex(index);

						setText(index);

						return true;
					}
				case Qt::Key_PageUp:
				case Qt::Key_PageDown:
					return false;
				case Qt::Key_Return:
				case Qt::Key_Enter:
					openUrl(m_lineEdit->text());
				case Qt::Key_Tab:
				case Qt::Key_Backtab:
				case Qt::Key_Escape:
				case Qt::Key_F4:
					if (keyEvent->key() == Qt::Key_F4 && !keyEvent->modifiers().testFlag(Qt::AltModifier))
					{
						break;
					}

					m_completionView->hide();
					m_completionView->deleteLater();
					m_completionView = NULL;

					m_lineEdit->setFocus();

					break;
				default:
					break;
			}

			return true;
		}
	}
	else if (object == m_completionView && event->type() == QEvent::MouseButtonPress && !m_completionView->viewport()->underMouse())
	{
		m_completionView->hide();
		m_completionView->deleteLater();
		m_completionView = NULL;

		m_lineEdit->setFocus();

		return true;
	}
	else if (m_completionView && object == m_completionView->viewport() && event->type() == QEvent::MouseMove)// && m_completionView->viewport()->underMouse())
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent)
		{
			const QModelIndex index = m_completionView->indexAt(mouseEvent->pos());

			if (index.isValid())
			{
				m_completionView->setCurrentIndex(index);
			}
		}
	}
	else if (object == m_completionView && (event->type() == QEvent::InputMethod || event->type() == QEvent::ShortcutOverride))
	{
		QApplication::sendEvent(m_lineEdit, event);
	}

	return QObject::eventFilter(object, event);
}

}
