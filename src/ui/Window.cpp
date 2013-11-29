#include "Window.h"
#include "ProgressBarWidget.h"
#include "../core/SettingsManager.h"
#include "../backends/web/WebBackend.h"
#include "../backends/web/WebBackendsManager.h"
#include "../backends/web/WebWidget.h"

#include "ui_Window.h"

namespace Otter
{

Window::Window(WebWidget *widget, QWidget *parent) : QWidget(parent),
	m_webWidget(widget),
	m_progressBarWidget(NULL),
	m_isPinned(false),
	m_ui(new Ui::Window)
{
	if (m_webWidget)
	{
		m_webWidget->setParent(this);
	}
	else
	{
		m_webWidget = WebBackendsManager::getBackend()->createWidget(false, this);
	}

	m_webWidget->setZoom(SettingsManager::getValue("Browser/DefaultZoom", 100).toInt());

	m_ui->setupUi(this);
	m_ui->backButton->setDefaultAction(getAction(GoBackAction));
	m_ui->forwardButton->setDefaultAction(getAction(GoForwardAction));
	m_ui->reloadOrStopButton->setDefaultAction(getAction(ReloadOrStopAction));
	m_ui->verticalLayout->insertWidget(2, m_webWidget);
	m_ui->addressLineEdit->setFocus();
	m_ui->findWidget->hide();

	connect(m_ui->addressLineEdit, SIGNAL(returnPressed()), this, SLOT(loadUrl()));
	connect(m_ui->findLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateFind()));
	connect(m_ui->caseSensitiveButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->highlightButton, SIGNAL(clicked()), this, SLOT(updateFindHighlight()));
	connect(m_ui->findNextButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->findPreviousButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->closeButton, SIGNAL(clicked()), m_ui->findWidget, SLOT(hide()));
	connect(m_webWidget, SIGNAL(requestedAddBookmark(QUrl)), this, SIGNAL(requestedAddBookmark(QUrl)));
	connect(m_webWidget, SIGNAL(requestedOpenUrl(QUrl,bool,bool)), this, SLOT(notifyRequestedOpenUrl(QUrl,bool,bool)));
	connect(m_webWidget, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
	connect(m_webWidget, SIGNAL(statusMessageChanged(QString,int)), this, SIGNAL(statusMessageChanged(QString,int)));
	connect(m_webWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_webWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_webWidget, SIGNAL(urlChanged(QUrl)), this, SLOT(updateUrl(QUrl)));
	connect(m_webWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_webWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_webWidget, SIGNAL(loadingChanged(bool)), this, SLOT(setLoading(bool)));
	connect(m_webWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
}

Window::~Window()
{
	delete m_ui;
}

void Window::print(QPrinter *printer)
{
	m_webWidget->print(printer);
}

void Window::triggerAction(WindowAction action, bool checked)
{
	if (action == FindAction)
	{
		m_ui->findWidget->setVisible(!m_ui->findWidget->isVisible());

		if (m_ui->findWidget->isVisible())
		{
			m_ui->findLineEdit->clear();
			m_ui->findLineEdit->setFocus();
		}
		else
		{
			updateFindHighlight();
		}
	}
	else if (action == FindNextAction || action == FindPreviousAction)
	{
		updateFind(action == FindPreviousAction);
	}
	else
	{
		m_webWidget->triggerAction(action, checked);
	}
}

void Window::setDefaultTextEncoding(const QString &encoding)
{
	m_webWidget->setDefaultTextEncoding(encoding);
}

void Window::setHistory(const HistoryInformation &history)
{
	m_webWidget->setHistory(history);
}

void Window::setZoom(int zoom)
{
	m_webWidget->setZoom(zoom);
}

void Window::setUrl(const QUrl &url)
{
	m_webWidget->setUrl(url);
}

void Window::setPinned(bool pinned)
{
	if (pinned != m_isPinned)
	{
		m_isPinned = pinned;

		emit isPinnedChanged(pinned);
	}
}

Window* Window::clone(QWidget *parent)
{
	if (!isClonable())
	{
		return NULL;
	}

	Window *window = new Window(m_webWidget->clone(), parent);
	window->setPinned(isPinned());

	return window;
}

QAction *Window::getAction(WindowAction action)
{
	return m_webWidget->getAction(action);
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

void Window::resizeEvent(QResizeEvent *event)
{
	updateProgressBarWidget();

	QWidget::resizeEvent(event);
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
	m_ui->addressLineEdit->setText(url.toString());
}

void Window::updateFind(bool backwards)
{
	if (sender() && sender()->objectName() == "findPreviousButton")
	{
		backwards = true;
	}

	FindFlags flags;

	if (backwards)
	{
		flags |= BackwardFind;
	}

	if (m_ui->caseSensitiveButton->isChecked())
	{
		flags |= CaseSensitiveFind;
	}

	QPalette palette = parentWidget()->palette();
	const bool found = m_webWidget->find(m_ui->findLineEdit->text(), flags);

	if (!m_ui->findLineEdit->text().isEmpty())
	{
		if (found)
		{
			palette.setColor(QPalette::Base, QColor("#CEF6DF"));
		}
		else
		{
			palette.setColor(QPalette::Base, QColor("#F1E7E4"));
		}
	}

	m_ui->findLineEdit->setPalette(palette);

	if (sender() && sender()->objectName() == "caseSensitiveButton")
	{
		m_webWidget->find(m_ui->findLineEdit->text(), (flags | BackwardFind));
	}

	updateFindHighlight();
}

void Window::updateFindHighlight()
{
	FindFlags flags = HighlightAllFind;

	if (m_ui->caseSensitiveButton->isChecked())
	{
		flags |= CaseSensitiveFind;
	}

	m_webWidget->find(QString(), flags);

	if (m_ui->highlightButton->isChecked() && m_ui->findWidget->isVisible())
	{
		m_webWidget->find(m_ui->findLineEdit->text(), flags);
	}
}

void Window::updateProgressBarWidget()
{
	if (!m_progressBarWidget)
	{
		return;
	}

	m_progressBarWidget->resize(QSize(width(), 30));
	m_progressBarWidget->move(QPoint(0, (height() - 30)));
}

void Window::setLoading(bool loading)
{
	if (loading && !m_progressBarWidget)
	{
		m_progressBarWidget = new ProgressBarWidget(m_webWidget, this);
		m_progressBarWidget->show();
		m_progressBarWidget->raise();
	}

	updateProgressBarWidget();
}

QUndoStack *Window::getUndoStack()
{
	return m_webWidget->getUndoStack();
}

QString Window::getDefaultTextEncoding() const
{
	return m_webWidget->getDefaultTextEncoding();
}

QString Window::getTitle() const
{
	return m_webWidget->getTitle();
}

QUrl Window::getUrl() const
{
	return m_webWidget->getUrl();
}

QIcon Window::getIcon() const
{
	return m_webWidget->getIcon();
}

HistoryInformation Window::getHistory() const
{
	return m_webWidget->getHistory();
}

int Window::getZoom() const
{
	return m_webWidget->getZoom();
}

bool Window::isClonable() const
{
	return true;
}

bool Window::isEmpty() const
{
	const QUrl url = m_webWidget->getUrl();

	return (url.scheme() == "about" && (url.path().isEmpty() || url.path() == "blank"));
}

bool Window::isLoading() const
{
	return m_webWidget->isLoading();
}

bool Window::isPinned() const
{
	return m_isPinned;
}

bool Window::isPrivate() const
{
	return m_webWidget->isPrivate();
}

}
