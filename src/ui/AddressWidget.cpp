#include "AddressWidget.h"
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

	if (SettingsManager::getValue("AddressField/ShowUrlIcon", false).toBool())
	{
		m_urlIconLabel = new QLabel(this);
		m_urlIconLabel->setAutoFillBackground(false);
		m_urlIconLabel->setFixedSize(16, 16);
		m_urlIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
		m_urlIconLabel->move(6, 4);

		setStyleSheet("QLineEdit {padding-left:22px;}");
	}

	connect(this, SIGNAL(returnPressed()), this, SLOT(notifyRequestedLoadUrl()));
}

void AddressWidget::notifyRequestedLoadUrl()
{
	emit requestedLoadUrl(getUrl());
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

	m_bookmarkLabel->setPixmap(Utils::getIcon("bookmarks").pixmap(m_bookmarkLabel->size(),( BookmarksManager::hasBookmark(url) ? QIcon::Active : QIcon::Disabled)));
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

void AddressWidget::setWindow(Window *window)
{
	m_window = window;

	if (window && m_urlIconLabel)
	{
		setIcon(window->getIcon());

		connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
	}
}

QUrl AddressWidget::getUrl() const
{
	return QUrl(text().isEmpty() ? "about:blank" : text());
}

}
