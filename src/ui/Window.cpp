#include "Window.h"
#include "../core/SettingsManager.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

#include "ui_Window.h"

#include <QtCore/QTimer>

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

	setContentsWidget(widget);

	m_ui->addressWidget->setWindow(this);

	connect(this, SIGNAL(aboutToClose()), m_contentsWidget, SLOT(close()));
	connect(m_ui->addressWidget, SIGNAL(requestedLoadUrl(QUrl)), this, SLOT(setUrl(QUrl)));
	connect(m_ui->addressWidget, SIGNAL(requestedSearch(QString,QString)), this, SLOT(search(QString,QString)));
	connect(m_ui->searchWidget, SIGNAL(requestedSearch(QString,QString)), this, SIGNAL(requestedSearch(QString,QString)));
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

void Window::close()
{
	emit aboutToClose();

	QTimer::singleShot(50, this, SLOT(notifyRequestedCloseWindow()));
}

void Window::search(const QString &query, const QString &engine)
{
	WebContentsWidget *widget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

	if (widget)
	{
		widget->search(query, engine);
	}
	else
	{
		widget = new WebContentsWidget(isPrivate(), NULL, this);
		widget->search(query, engine);

		setContentsWidget(widget);
	}
}

void Window::notifyRequestedCloseWindow()
{
	emit requestedCloseWindow(this);
}

void Window::notifyRequestedOpenUrl(const QUrl &url, bool background, bool newWindow)
{
	emit requestedOpenUrl(url, isPrivate(), background, newWindow);
}

void Window::setDefaultTextEncoding(const QString &encoding)
{
	if (m_contentsWidget->getType() == QLatin1String("web"))
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget)
		{
			return webWidget->setDefaultTextEncoding(encoding);
		}
	}
}

void Window::setSearchEngine(const QString &engine)
{
	m_ui->searchWidget->setCurrentSearchEngine(engine);
}

void Window::setUrl(const QUrl &url)
{
	ContentsWidget *newWidget = NULL;

	if (url.scheme() == QLatin1String("about"))
	{
		if (!url.path().isEmpty() && url.path() != QLatin1String("blank") && SessionsManager::hasUrl(url, true))
		{
			m_ui->addressWidget->setUrl(m_contentsWidget->getUrl());

			return;
		}

		if (url.path() == QLatin1String("bookmarks"))
		{
			newWidget = new BookmarksContentsWidget(this);
		}
		else if (url.path() == QLatin1String("cache"))
		{
			newWidget = new CacheContentsWidget(this);
		}
		else if (url.path() == QLatin1String("config"))
		{
			newWidget = new ConfigurationContentsWidget(this);
		}
		else if (url.path() == QLatin1String("cookies"))
		{
			newWidget = new CookiesContentsWidget(this);
		}
		else if (url.path() == QLatin1String("history"))
		{
			newWidget = new HistoryContentsWidget(this);
		}
		else if (url.path() == QLatin1String("transfers"))
		{
			newWidget = new TransfersContentsWidget(this);
		}
	}

	if (!newWidget && m_contentsWidget->getType() != QLatin1String("web"))
	{
		newWidget = new WebContentsWidget(false, NULL, this);
	}

	if (newWidget)
	{
		setContentsWidget(newWidget);
	}

	m_ui->addressWidget->setUrl(url);

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

	m_ui->navigationWidget->setVisible(m_contentsWidget->getType() == QLatin1String("web"));
	m_ui->backButton->setDefaultAction(m_contentsWidget->getAction(GoBackAction));
	m_ui->forwardButton->setDefaultAction(m_contentsWidget->getAction(GoForwardAction));
	m_ui->reloadOrStopButton->setDefaultAction(m_contentsWidget->getAction(ReloadOrStopAction));
	m_ui->addressWidget->setUrl(m_contentsWidget->getUrl());
	m_ui->addressWidget->setFocus();

	emit actionsChanged();
	emit canZoomChanged(m_contentsWidget->canZoom());
	emit titleChanged(m_contentsWidget->getTitle());
	emit iconChanged(m_contentsWidget->getIcon());

	connect(m_contentsWidget, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString)));
	connect(m_contentsWidget, SIGNAL(requestedOpenUrl(QUrl,bool,bool,bool)), this, SIGNAL(requestedOpenUrl(QUrl,bool,bool,bool)));
	connect(m_contentsWidget, SIGNAL(requestedNewWindow(ContentsWidget*)), this, SIGNAL(requestedNewWindow(ContentsWidget*)));
	connect(m_contentsWidget, SIGNAL(requestedSearch(QString,QString)), this, SIGNAL(requestedSearch(QString,QString)));
	connect(m_contentsWidget, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
	connect(m_contentsWidget, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
	connect(m_contentsWidget, SIGNAL(statusMessageChanged(QString,int)), this, SIGNAL(statusMessageChanged(QString,int)));
	connect(m_contentsWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_contentsWidget, SIGNAL(urlChanged(QUrl)), m_ui->addressWidget, SLOT(setUrl(QUrl)));
	connect(m_contentsWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_contentsWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_contentsWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_contentsWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
}

Window* Window::clone(QWidget *parent)
{
	if (!canClone())
	{
		return NULL;
	}

	Window *window = new Window(false, m_contentsWidget->clone(), parent);
	window->setSearchEngine(getSearchEngine());
	window->setPinned(isPinned());

	return window;
}

ContentsWidget* Window::getContentsWidget()
{
	return m_contentsWidget;
}

QString Window::getDefaultTextEncoding() const
{
	if (m_contentsWidget->getType() == QLatin1String("web"))
	{
		WebContentsWidget *webWidget = qobject_cast<WebContentsWidget*>(m_contentsWidget);

		if (webWidget)
		{
			return webWidget->getDefaultTextEncoding();
		}
	}

	return QString();
}

QString Window::getSearchEngine() const
{
	return m_ui->searchWidget->getCurrentSearchEngine();
}

QString Window::getTitle() const
{
	return m_contentsWidget->getTitle();
}

QLatin1String Window::getType() const
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

bool Window::canClone() const
{
	return m_contentsWidget->canClone();
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
