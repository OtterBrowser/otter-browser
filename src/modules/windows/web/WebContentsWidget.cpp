#include "WebContentsWidget.h"
#include "ProgressBarWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"
#include "../../../ui/WebWidget.h"

#include "ui_WebContentsWidget.h"

namespace Otter
{

WebContentsWidget::WebContentsWidget(bool privateWindow, WebWidget *widget, Window *window) : ContentsWidget(window),
	m_webWidget(widget),
	m_progressBarWidget(NULL),
	m_ui(new Ui::WebContentsWidget)
{
	if (m_webWidget)
	{
		m_webWidget->setParent(this);
	}
	else
	{
		m_webWidget = WebBackendsManager::getBackend()->createWidget(privateWindow, this);
	}

	m_ui->setupUi(this);
	m_ui->findWidget->hide();
	m_ui->verticalLayout->addWidget(m_webWidget);

	connect(m_ui->findLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateFind()));
	connect(m_ui->caseSensitiveButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->highlightButton, SIGNAL(clicked()), this, SLOT(updateFindHighlight()));
	connect(m_ui->findNextButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->findPreviousButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->closeButton, SIGNAL(clicked()), m_ui->findWidget, SLOT(hide()));
	connect(m_webWidget, SIGNAL(requestedAddBookmark(QUrl)), this, SIGNAL(requestedAddBookmark(QUrl)));
	connect(m_webWidget, SIGNAL(requestedOpenUrl(QUrl,bool,bool)), this, SLOT(notifyRequestedOpenUrl(QUrl,bool,bool)));
	connect(m_webWidget, SIGNAL(requestedNewWindow(WebWidget*)), this, SLOT(notifyRequestedNewWindow(WebWidget*)));
	connect(m_webWidget, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
	connect(m_webWidget, SIGNAL(statusMessageChanged(QString,int)), this, SIGNAL(statusMessageChanged(QString,int)));
	connect(m_webWidget, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged(QString)));
	connect(m_webWidget, SIGNAL(urlChanged(QUrl)), this, SIGNAL(urlChanged(QUrl)));
	connect(m_webWidget, SIGNAL(iconChanged(QIcon)), this, SIGNAL(iconChanged(QIcon)));
	connect(m_webWidget, SIGNAL(loadingChanged(bool)), this, SIGNAL(loadingChanged(bool)));
	connect(m_webWidget, SIGNAL(loadingChanged(bool)), this, SLOT(setLoading(bool)));
	connect(m_webWidget, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
}

WebContentsWidget::~WebContentsWidget()
{
	delete m_ui;
}

void WebContentsWidget::changeEvent(QEvent *event)
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

void WebContentsWidget::resizeEvent(QResizeEvent *event)
{
	updateProgressBarWidget();

	ContentsWidget::resizeEvent(event);
}

void WebContentsWidget::search(const QString &search, const QString &query)
{
	m_webWidget->search(search, query);
}

void WebContentsWidget::print(QPrinter *printer)
{
	m_webWidget->print(printer);
}

void WebContentsWidget::triggerAction(WindowAction action, bool checked)
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

void WebContentsWidget::setDefaultTextEncoding(const QString &encoding)
{
	m_webWidget->setDefaultTextEncoding(encoding);
}

void WebContentsWidget::setHistory(const HistoryInformation &history)
{
	m_webWidget->setHistory(history);
}

void WebContentsWidget::setZoom(int zoom)
{
	m_webWidget->setZoom(zoom);
}

void WebContentsWidget::setUrl(const QUrl &url)
{
	m_webWidget->setUrl(url);
}

void WebContentsWidget::notifyRequestedOpenUrl(const QUrl &url, bool background, bool newWindow)
{
	emit requestedOpenUrl(url, isPrivate(), background, newWindow);
}

void WebContentsWidget::notifyRequestedNewWindow(WebWidget *widget)
{
	emit requestedNewWindow(new WebContentsWidget(isPrivate(), widget, NULL));
}

void WebContentsWidget::updateFind(bool backwards)
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

void WebContentsWidget::updateFindHighlight()
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

void WebContentsWidget::updateProgressBarWidget()
{
	if (!m_progressBarWidget)
	{
		return;
	}

	m_progressBarWidget->resize(QSize(width(), 30));
	m_progressBarWidget->move(QPoint(0, (height() - 30)));
}

void WebContentsWidget::setLoading(bool loading)
{
	if (loading && !m_progressBarWidget)
	{
		m_progressBarWidget = new ProgressBarWidget(m_webWidget, this);
		m_progressBarWidget->show();
		m_progressBarWidget->raise();
	}

	updateProgressBarWidget();
}

WebContentsWidget* WebContentsWidget::clone(Window *parent)
{
	if (!isClonable())
	{
		return NULL;
	}

	return new WebContentsWidget(m_webWidget->isPrivate(), m_webWidget->clone(), parent);
}

QAction *WebContentsWidget::getAction(WindowAction action)
{
	return m_webWidget->getAction(action);
}

QUndoStack *WebContentsWidget::getUndoStack()
{
	return m_webWidget->getUndoStack();
}

QString WebContentsWidget::getDefaultTextEncoding() const
{
	return m_webWidget->getDefaultTextEncoding();
}

QString WebContentsWidget::getTitle() const
{
	return m_webWidget->getTitle();
}

QString WebContentsWidget::getType() const
{
	return "web";
}

QUrl WebContentsWidget::getUrl() const
{
	return m_webWidget->getUrl();
}

QIcon WebContentsWidget::getIcon() const
{
	return m_webWidget->getIcon();
}

QPixmap WebContentsWidget::getThumbnail() const
{
	return m_webWidget->getThumbnail();
}

HistoryInformation WebContentsWidget::getHistory() const
{
	return m_webWidget->getHistory();
}

int WebContentsWidget::getZoom() const
{
	return m_webWidget->getZoom();
}

bool WebContentsWidget::canZoom() const
{
	return true;
}

bool WebContentsWidget::isClonable() const
{
	return true;
}

bool WebContentsWidget::isLoading() const
{
	return m_webWidget->isLoading();
}

bool WebContentsWidget::isPrivate() const
{
	return m_webWidget->isPrivate();
}

}
