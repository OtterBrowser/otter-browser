#include "AddressWidget.h"
#include "BookmarkPropertiesDialog.h"
#include "Window.h"
#include "../core/BookmarksManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

namespace Otter
{

AddressWidget::AddressWidget(QWidget *parent) : QLineEdit(parent),
	m_window(NULL),
	m_urlIconLabel(NULL),
	m_bookmarkLabel(new QLabel(this))
{
	m_bookmarkLabel->setAutoFillBackground(false);
	m_bookmarkLabel->setFixedSize(16, 16);
	m_bookmarkLabel->move((width() - 22), 4);
	m_bookmarkLabel->setPixmap(Utils::getIcon("bookmarks").pixmap(m_bookmarkLabel->size(), QIcon::Disabled));
	m_bookmarkLabel->setCursor(Qt::ArrowCursor);
	m_bookmarkLabel->installEventFilter(this);

	optionChanged("AddressField/ShowUrlIcon", SettingsManager::getValue("AddressField/ShowUrlIcon"));

	connect(this, SIGNAL(returnPressed()), this, SLOT(notifyRequestedLoadUrl()));
	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateBookmark()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void AddressWidget::notifyRequestedLoadUrl()
{
	emit requestedLoadUrl(getUrl());
}

void AddressWidget::updateBookmark()
{
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

void AddressWidget::resizeEvent(QResizeEvent *event)
{
	QLineEdit::resizeEvent(event);

	m_bookmarkLabel->move((width() - 22), ((height() - m_bookmarkLabel->height()) / 2));

	if (m_urlIconLabel)
	{
		m_urlIconLabel->move(6, ((height() - m_urlIconLabel->height()) / 2));
	}
}

void AddressWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == "AddressField/ShowUrlIcon")
	{
		if (value.toBool() && !m_urlIconLabel)
		{
			m_urlIconLabel = new QLabel(this);
			m_urlIconLabel->setAutoFillBackground(false);
			m_urlIconLabel->setFixedSize(16, 16);
			m_urlIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
			m_urlIconLabel->move(6, 4);

			setStyleSheet("QLineEdit {padding-left:22px;}");
		}
		else if (!value.toBool() && m_urlIconLabel)
		{
			m_urlIconLabel->deleteLater();
			m_urlIconLabel = NULL;
		}
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
	return QUrl(text().isEmpty() ? "about:blank" : text());
}

bool AddressWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_bookmarkLabel && event->type() == QEvent::MouseButtonPress && m_bookmarkLabel->isEnabled())
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

	return QLineEdit::eventFilter(object, event);
}

}
