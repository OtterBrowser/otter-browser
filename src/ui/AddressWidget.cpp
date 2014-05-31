/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "BookmarkPropertiesDialog.h"
#include "Window.h"
#include "../core/AddressCompletionModel.h"
#include "../core/BookmarksManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionFrame>

namespace Otter
{

AddressWidget::AddressWidget(QWidget *parent) : QLineEdit(parent),
	m_window(NULL),
	m_completer(new QCompleter(AddressCompletionModel::getInstance(), this)),
	m_bookmarkLabel(NULL),
	m_urlIconLabel(NULL),
	m_lookupID(0),
	m_lookupTimer(0)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::InlineCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);

	optionChanged(QLatin1String("AddressField/ShowBookmarkIcon"), SettingsManager::getValue(QLatin1String("AddressField/ShowBookmarkIcon")));
	optionChanged(QLatin1String("AddressField/ShowUrlIcon"), SettingsManager::getValue(QLatin1String("AddressField/ShowUrlIcon")));
	setCompleter(m_completer);
	setMinimumWidth(100);
	setMouseTracking(true);

	connect(this, SIGNAL(returnPressed()), this, SLOT(notifyRequestedLoadUrl()));
	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateBookmark()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void AddressWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_lookupTimer)
	{
		QHostInfo::abortHostLookup(m_lookupID);

		killTimer(m_lookupTimer);

		m_lookupTimer = 0;

		emit requestedSearch(m_lookupQuery, SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString());

		m_lookupQuery.clear();
	}
}

void AddressWidget::paintEvent(QPaintEvent *event)
{
	QLineEdit::paintEvent(event);

	QColor badgeColor = QColor(245, 245, 245);
	QStyleOptionFrame panel;

	initStyleOption(&panel);

	panel.palette = palette();
	panel.palette.setColor(QPalette::Base, badgeColor);
	panel.state = QStyle::State_Active;

	QRect rectangle = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	rectangle.setWidth(30);
	rectangle.moveTo(panel.lineWidth, panel.lineWidth);

	m_securityBadgeRectangle = rectangle;

	QPainter painter(this);
	painter.fillRect(rectangle, badgeColor);
	painter.setClipRect(rectangle);

	style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

	QPalette linePalette = palette();
	linePalette.setCurrentColorGroup(QPalette::Disabled);

	painter.setPen(QPen(linePalette.mid().color(), 1));
	painter.drawLine(rectangle.right(), rectangle.top(), rectangle.right(), rectangle.bottom());
}

void AddressWidget::resizeEvent(QResizeEvent *event)
{
	QLineEdit::resizeEvent(event);

	if (m_bookmarkLabel)
	{
		m_bookmarkLabel->move((width() - 22), ((height() - m_bookmarkLabel->height()) / 2));
	}

	if (m_urlIconLabel)
	{
		m_urlIconLabel->move(36, ((height() - m_urlIconLabel->height()) / 2));
	}
}

void AddressWidget::focusInEvent(QFocusEvent *event)
{
	QLineEdit::focusInEvent(event);

	if (!text().trimmed().isEmpty() && SettingsManager::getValue(QLatin1String("AddressField/SelectAllOnFocus")).toBool())
	{
		QTimer::singleShot(0, this, SLOT(selectAll()));
	}
}

void AddressWidget::mouseMoveEvent(QMouseEvent *event)
{
	QLineEdit::mouseMoveEvent(event);

	if (m_securityBadgeRectangle.contains(event->pos()))
	{
		setCursor(Qt::ArrowCursor);
	}
	else
	{
		setCursor(Qt::IBeamCursor);
	}
}

void AddressWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (text().isEmpty() && event->button() == Qt::MiddleButton && !QApplication::clipboard()->text().isEmpty())
	{
		handleUserInput(QApplication::clipboard()->text().trimmed());

		event->accept();
	}
	else
	{
		QLineEdit::mouseReleaseEvent(event);
	}
}

void AddressWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		selectAll();

		event->accept();
	}
	else
	{
		QLineEdit::mouseDoubleClickEvent(event);
	}
}

void AddressWidget::handleUserInput(const QString &text)
{
	const BookmarkInformation *bookmark = BookmarksManager::getBookmarkByKeyword(text);

	if (bookmark)
	{
		WindowsManager *windowsManager = SessionsManager::getWindowsManager();

		if (windowsManager)
		{
			windowsManager->open(bookmark);

			return;
		}
	}

	if (text == QString(QLatin1Char('~')) || text.startsWith(QLatin1Char('~') + QDir::separator()))
	{
		const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

		if (!locations.isEmpty())
		{
			emit requestedLoadUrl(QUrl(locations.first() + text.mid(1)));

			return;
		}
	}

	const QUrl url = QUrl::fromUserInput(text);

	if (url.isValid() && (url.isLocalFile() || QRegularExpression(QLatin1String("^(\\w+\\:\\S+)|([\\w\\-]+\\.[a-zA-Z]{2,}(/\\S*)?$)")).match(text).hasMatch()))
	{
		emit requestedLoadUrl(url);

		return;
	}

	const QString shortcut = text.section(QLatin1Char(' '), 0, 0);
	const QStringList engines = SearchesManager::getSearchEngines();
	SearchInformation *engine = NULL;

	for (int i = 0; i < engines.count(); ++i)
	{
		engine = SearchesManager::getSearchEngine(engines.at(i));

		if (engine && shortcut == engine->shortcut)
		{
			emit requestedSearch(text.section(QLatin1Char(' '), 1), engine->identifier);

			return;
		}
	}

	const int lookupTimeout = SettingsManager::getValue(QLatin1String("AddressField/HostLookupTimeout")).toInt();

	if (url.isValid() && lookupTimeout > 0)
	{
		if (text == m_lookupQuery)
		{
			return;
		}

		m_lookupQuery = text;

		if (m_lookupTimer != 0)
		{
			QHostInfo::abortHostLookup(m_lookupID);

			killTimer(m_lookupTimer);

			m_lookupTimer = 0;
		}

		m_lookupID = QHostInfo::lookupHost(url.host(), this, SLOT(verifyLookup(QHostInfo)));

		m_lookupTimer = startTimer(lookupTimeout);

		return;
	}

	emit requestedSearch(text, SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString());
}

void AddressWidget::removeIcon()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		SettingsManager::setValue(QStringLiteral("AddressField/Show%1Icon").arg(action->data().toString()), false);
	}
}

void AddressWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("AddressField/ShowBookmarkIcon"))
	{
		if (value.toBool() && !m_bookmarkLabel)
		{
			m_bookmarkLabel = new QLabel(this);
			m_bookmarkLabel->setObjectName(QLatin1String("Bookmark"));
			m_bookmarkLabel->setAutoFillBackground(false);
			m_bookmarkLabel->setFixedSize(16, 16);
			m_bookmarkLabel->move((width() - 22), 4);
			m_bookmarkLabel->setPixmap(Utils::getIcon(QLatin1String("bookmarks")).pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
			m_bookmarkLabel->setCursor(Qt::ArrowCursor);
			m_bookmarkLabel->installEventFilter(this);
		}
		else if (!value.toBool() && m_bookmarkLabel)
		{
			m_bookmarkLabel->deleteLater();
			m_bookmarkLabel = NULL;
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
			m_urlIconLabel->move(36, 4);
			m_urlIconLabel->installEventFilter(this);

			QMargins margins = textMargins();
			margins.setLeft(52);

			setTextMargins(margins);
		}
		else
		{
			if (!value.toBool() && m_urlIconLabel)
			{
				m_urlIconLabel->deleteLater();
				m_urlIconLabel = NULL;
			}

			QMargins margins = textMargins();
			margins.setLeft(30);

			setTextMargins(margins);
		}
	}
}

void AddressWidget::verifyLookup(const QHostInfo &host)
{
	killTimer(m_lookupTimer);

	m_lookupTimer = 0;

	if (host.error() == QHostInfo::NoError)
	{
		emit requestedLoadUrl(getUrl());
	}
	else
	{
		emit requestedSearch(m_lookupQuery, SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString());
	}

	m_lookupQuery.clear();
}

void AddressWidget::notifyRequestedLoadUrl()
{
	handleUserInput(text().trimmed());
}

void AddressWidget::updateBookmark()
{
	if (!m_bookmarkLabel)
	{
		return;
	}

	const QUrl url = getUrl();

	if (url.scheme() == QLatin1String("about"))
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

void AddressWidget::setIcon(const QIcon &icon)
{
	if (m_urlIconLabel)
	{
		m_urlIconLabel->setPixmap(icon.pixmap(m_urlIconLabel->size()));
	}
}

void AddressWidget::setUrl(const QUrl &url)
{
	updateBookmark();

	if (m_window && url.scheme() != QLatin1String("javascript"))
	{
		if (!hasFocus())
		{
			setText((url.scheme() == QLatin1String("about") && m_window->isUrlEmpty()) ? QString() : url.toString());
		}

		setIcon(m_window->getIcon());
	}
}

void AddressWidget::setWindow(Window *window)
{
	m_window = window;

	if (window && m_urlIconLabel)
	{
		setIcon(window->getIcon());
		setUrl(window->getUrl());

		connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
	}
}

QUrl AddressWidget::getUrl() const
{
	return QUrl(text().isEmpty() ? QLatin1String("about:blank") : text());
}

bool AddressWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_bookmarkLabel && m_bookmarkLabel && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			if (m_bookmarkLabel->isEnabled())
			{
				if (BookmarksManager::hasBookmark(getUrl()))
				{
					BookmarksManager::deleteBookmark(getUrl());
				}
				else
				{
					BookmarkInformation *bookmark = new BookmarkInformation();
					bookmark->url = getUrl().toString(QUrl::RemovePassword);
					bookmark->title = m_window->getTitle();
					bookmark->type = UrlBookmark;

					BookmarkPropertiesDialog dialog(bookmark, -1, this);

					if (dialog.exec() == QDialog::Rejected)
					{
						delete bookmark;
					}
				}

				updateBookmark();
			}

			return true;
		}
	}

	if (object && event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent*>(event);

		if (contextMenuEvent)
		{
			QMenu menu(this);
			QAction *action = menu.addAction(tr("Remove This Icon"), this, SLOT(removeIcon()));
			action->setData(object->objectName());

			menu.exec(contextMenuEvent->globalPos());

			contextMenuEvent->accept();

			return true;
		}
	}

	return QLineEdit::eventFilter(object, event);
}

}
