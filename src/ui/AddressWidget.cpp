#include "AddressWidget.h"
#include "BookmarkPropertiesDialog.h"
#include "Window.h"
#include "../core/BookmarksManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include <QtCore/QRegularExpression>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

AddressWidget::AddressWidget(QWidget *parent) : QLineEdit(parent),
	m_window(NULL),
	m_completer(new QCompleter(this)),
	m_bookmarkLabel(NULL),
	m_urlIconLabel(NULL)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::InlineCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);

	optionChanged("AddressField/SuggestBookmarks", SettingsManager::getValue("AddressField/SuggestBookmarks"));
	optionChanged("AddressField/ShowBookmarkIcon", SettingsManager::getValue("AddressField/ShowBookmarkIcon"));
	optionChanged("AddressField/ShowUrlIcon", SettingsManager::getValue("AddressField/ShowUrlIcon"));
	setCompleter(m_completer);

	connect(this, SIGNAL(returnPressed()), this, SLOT(notifyRequestedLoadUrl()));
	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateBookmark()));
	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateCompletion()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
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
		m_urlIconLabel->move(6, ((height() - m_urlIconLabel->height()) / 2));
	}
}

void AddressWidget::removeIcon()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action && SettingsManager::contains(QString("AddressField/Show%1Icon").arg(action->data().toString())))
	{
		SettingsManager::setValue(QString("AddressField/Show%1Icon").arg(action->data().toString()), false);
	}
}

void AddressWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == "AddressField/SuggestBookmarks")
	{
		if (value.toBool() && (!m_completer->model() || m_completer->model()->rowCount() == 0))
		{
			QStandardItemModel *model = new QStandardItemModel(m_completer);
			const QStringList bookmarks = BookmarksManager::getUrls();

			for (int i = 0; i < bookmarks.count(); ++i)
			{
				model->appendRow(new QStandardItem(bookmarks.at(i)));
			}

			QStringList moduleUrls;
			moduleUrls << "about:bookmarks" << "about:config" << "about:cookies" << "about:transfers";

			for (int i = 0; i < moduleUrls.count(); ++i)
			{
				model->appendRow(new QStandardItem(moduleUrls.at(i)));
			}

			m_completer->setModel(model);
		}
		else if (!value.toBool() && m_completer->model() && m_completer->model()->rowCount() > 0)
		{
			m_completer->model()->removeRows(0, m_completer->model()->rowCount());
		}
	}
	else if (option == "AddressField/ShowBookmarkIcon")
	{
		if (value.toBool() && !m_bookmarkLabel)
		{
			m_bookmarkLabel = new QLabel(this);
			m_bookmarkLabel->setObjectName("Bookmark");
			m_bookmarkLabel->setAutoFillBackground(false);
			m_bookmarkLabel->setFixedSize(16, 16);
			m_bookmarkLabel->move((width() - 22), 4);
			m_bookmarkLabel->setPixmap(Utils::getIcon("bookmarks").pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
			m_bookmarkLabel->setCursor(Qt::ArrowCursor);
			m_bookmarkLabel->installEventFilter(this);
		}
		else if (!value.toBool() && m_bookmarkLabel)
		{
			m_bookmarkLabel->deleteLater();
			m_bookmarkLabel = NULL;
		}
	}
	else if (option == "AddressField/ShowUrlIcon")
	{
		if (value.toBool() && !m_urlIconLabel)
		{
			m_urlIconLabel = new QLabel(this);
			m_urlIconLabel->setObjectName("Url");
			m_urlIconLabel->setAutoFillBackground(false);
			m_urlIconLabel->setFixedSize(16, 16);
			m_urlIconLabel->move(6, 4);
			m_urlIconLabel->installEventFilter(this);

			setStyleSheet("QLineEdit {padding-left:22px;}");
		}
		else if (!value.toBool() && m_urlIconLabel)
		{
			m_urlIconLabel->deleteLater();
			m_urlIconLabel = NULL;

			setStyleSheet(QString());
		}
	}
}

void AddressWidget::notifyRequestedLoadUrl()
{
	if (QRegularExpression(QString("^(%1) .+$").arg(SearchesManager::getSearchShortcuts().join('|'))).match(text()).hasMatch())
	{
		const QStringList engines = SearchesManager::getSearchEngines();
		const QString shortcut = text().section(' ', 0, 0);

		for (int i = 0; i < engines.count(); ++i)
		{
			SearchInformation *search = SearchesManager::getSearchEngine(engines.at(i));

			if (search && shortcut == search->shortcut)
			{
				emit requestedSearch(text().section(' ', 1), search->identifier);

				return;
			}
		}

		return;
	}

	emit requestedLoadUrl(getUrl());
}

void AddressWidget::updateBookmark()
{
	if (!m_bookmarkLabel)
	{
		return;
	}

	const QUrl url = getUrl();

	if (url.scheme() == "about")
	{
		m_bookmarkLabel->setEnabled(false);
		m_bookmarkLabel->setPixmap(Utils::getIcon("bookmarks").pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
		m_bookmarkLabel->setToolTip(QString());

		return;
	}

	const bool hasBookmark = BookmarksManager::hasBookmark(url);

	m_bookmarkLabel->setEnabled(true);
	m_bookmarkLabel->setPixmap(Utils::getIcon("bookmarks").pixmap(m_bookmarkLabel->size(), (hasBookmark ? QIcon::Active : QIcon::Disabled)));
	m_bookmarkLabel->setToolTip(hasBookmark ? tr("Remove Bookmark") : tr("Add Bookmark"));
}

void AddressWidget::updateCompletion()
{
	if (SettingsManager::getValue("AddressField/SuggestBookmarks").toBool())
	{
		m_completer->model()->removeRows(0, m_completer->model()->rowCount());

		optionChanged("AddressField/SuggestBookmarks", true);
	}
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
	setText((url.scheme() == "about" && url.path() == "blank") ? QString() : url.toString());
	updateBookmark();
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
	return QUrl(text().isEmpty() ? "about:blank" : text());
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
