#include "Window.h"
#include "../core/SettingsManager.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

#include "ui_Window.h"

namespace Otter
{

Window::Window(bool privateWindow, ContentsWidget *widget, QWidget *parent) : QWidget(parent),
	m_contentsWidget(NULL),
	m_isPinned(false),
	m_ui(new Ui::Window)
{
	m_ui->setupUi(this);

	if (widget)
	{
		widget->setParent(this);
	}
	else
	{
		widget = new WebContentsWidget(privateWindow, NULL, this);
	}

	widget->setZoom(SettingsManager::getValue("Browser/DefaultZoom", 100).toInt());

	setContentsWidget(widget);
}

Window::~Window()
{
	delete m_ui;
}

void Window::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void Window::print(QPrinter *printer)
{
	m_contentsWidget->print(printer);
}

void Window::triggerAction(WindowAction action, bool checked)
{
	m_contentsWidget->triggerAction(action, checked);
}

void Window::loadUrl()
{
	setUrl(QUrl(m_ui->addressLineEdit->text()));
}

void Window::notifyRequestedOpenUrl(const QUrl &url, bool background, bool newWindow)
{
	emit requestedOpenUrl(url, isPrivate(), background, newWindow);
}

void Window::updateUrl(const QUrl &url)
{
	m_ui->addressLineEdit->setText((url.scheme() == "about" && url.path() == "blank") ? QString() : url.toString());
}

void Window::setDefaultTextEncoding(const QString &encoding)
{
	if (m_contentsWidget->getType() == "web")
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget)
		{
			return webWidget->setDefaultTextEncoding(encoding);
		}
	}
}

void Window::setHistory(const HistoryInformation &history)
{
	m_contentsWidget->setHistory(history);
}

void Window::setZoom(int zoom)
{
	m_contentsWidget->setZoom(zoom);
}

void Window::setUrl(const QUrl &url)
{
	ContentsWidget *newWidget = NULL;

	if (url.scheme() == "about")
	{
		if (url.path() == "cookies")
		{
			newWidget = new CookiesContentsWidget(this);
		}
	}

	if (!newWidget && m_contentsWidget->getType() != "web")
	{
		newWidget = new WebContentsWidget(false, NULL, this);
	}

	if (newWidget)
	{
		setContentsWidget(newWidget);
	}

	m_contentsWidget->setUrl(url);
}

void Window::setPinned(bool pinned)
{
	if (pinned != m_isPinned)
	{
		m_isPinned = pinned;

		emit isPinnedChanged(pinned);
	}
}

void Window::setContentsWidget(ContentsWidget *widget)
{
	if (m_contentsWidget)
	{
		layout()->removeWidget(m_contentsWidget);

		m_contentsWidget->deleteLater();
	}

	m_contentsWidget = widget;

	layout()->addWidget(m_contentsWidget);

	m_ui->backButton->setDefaultAction(getAction(GoBackAction));
	m_ui->forwardButton->setDefaultAction(getAction(GoForwardAction));
	m_ui->reloadOrStopButton->setDefaultAction(getAction(ReloadOrStopAction));
	m_ui->addressLineEdit->setFocus();

	updateUrl(m_contentsWidget->getUrl());

	emit actionsChanged();
	emit canZoomChanged(m_contentsWidget->canZoom());
	emit titleChanged(m_contentsWidget->getTitle());
	emit iconChanged(m_contentsWidget->getIcon());

	connect(m_ui->addressLineEdit, SIGNAL(returnPressed()), this, SLOT(loadUrl()));
	connect(m_contentsWidget, SIGNAL(requestedAddBookmark(QUrl)), this, SIGNAL(requestedAddBookmark(QUrl)));
	connect(m_contentsWidget, SIGNAL(requestedOpenUrl(QUrl,bool,bool,bool)), this, SIGNAL(requestedOpenUrl(QUrl,bool,bool,bool)));
	connect(m_contentsWidget, SIGNAL(requestedNewWindow(ContentsWidget*)), this, SIGNAL(requestedNewWindow(ContentsWidget*)));
	connect(m_contentsWidget, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
	connect(m_contentsWidget, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
	connect(m_contentsWidget, SIGNAL(statusMessageChanged(QString,int)), this, SIGNAL(statusMessageChanged(QString,int)));
	connect(m_contentsWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), this, SLOT(updateUrl(QUrl)));
	connect(m_contentsWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_contentsWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_contentsWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_contentsWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
}

Window* Window::clone(QWidget *parent)
{
	if (!isClonable())
	{
		return NULL;
	}

	Window *window = new Window(false, m_contentsWidget->clone(), parent);
	window->setPinned(isPinned());

	return window;
}

QAction *Window::getAction(WindowAction action)
{
	return m_contentsWidget->getAction(action);
}

QUndoStack *Window::getUndoStack()
{
	return m_contentsWidget->getUndoStack();
}

QString Window::getDefaultTextEncoding() const
{
	if (m_contentsWidget->getType() == "web")
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget)
		{
			return webWidget->getDefaultTextEncoding();
		}
	}

	return QString();
}

QString Window::getTitle() const
{
	return m_contentsWidget->getTitle();
}

QString Window::getType() const
{
	return m_contentsWidget->getType();
}

QUrl Window::getUrl() const
{
	return m_contentsWidget->getUrl();
}

QIcon Window::getIcon() const
{
	return m_contentsWidget->getIcon();
}

QPixmap Window::getThumbnail() const
{
	return m_contentsWidget->getThumbnail();
}

HistoryInformation Window::getHistory() const
{
	return m_contentsWidget->getHistory();
}

int Window::getZoom() const
{
	return m_contentsWidget->getZoom();
}

bool Window::canZoom() const
{
	return m_contentsWidget->canZoom();
}

bool Window::isClonable() const
{
	return m_contentsWidget->isClonable();
}

bool Window::isLoading() const
{
	return m_contentsWidget->isLoading();
}

bool Window::isPinned() const
{
	return m_isPinned;
}

bool Window::isPrivate() const
{
	return m_contentsWidget->isPrivate();
}

}
