/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebContentsWidget.h"
#include "ProgressBarWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/WebBackend.h"
#include "../../../core/WebBackendsManager.h"
#include "../../../ui/WebWidget.h"

#include "ui_WebContentsWidget.h"

#include <QtGui/QMouseEvent>

namespace Otter
{

QString WebContentsWidget::m_quickFindQuery = NULL;

WebContentsWidget::WebContentsWidget(bool privateWindow, WebWidget *widget, Window *window) : ContentsWidget(window),
	m_webWidget(widget),
	m_progressBarWidget(NULL),
	m_progressBarTimer(0),
	m_showProgressBar(true),
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

	setFocusPolicy(Qt::StrongFocus);

	m_ui->setupUi(this);
	m_ui->findWidget->hide();
	m_ui->findWidget->installEventFilter(this);
	m_ui->verticalLayout->addWidget(m_webWidget);

	optionChanged(QLatin1String("Browser/ShowDetailedProgressBar"), SettingsManager::getValue(QLatin1String("Browser/ShowDetailedProgressBar")));
	optionChanged(QLatin1String("Search/EnableFindInPageAsYouType"), SettingsManager::getValue(QLatin1String("Search/EnableFindInPageAsYouType")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(m_ui->caseSensitiveButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->highlightButton, SIGNAL(clicked()), this, SLOT(updateFindHighlight()));
	connect(m_ui->findNextButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->findPreviousButton, SIGNAL(clicked()), this, SLOT(updateFind()));
	connect(m_ui->closeButton, SIGNAL(clicked()), m_ui->findWidget, SLOT(hide()));
	connect(m_ui->closeButton, SIGNAL(clicked()), m_ui->findLineEdit, SLOT(clear()));
	connect(m_webWidget, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString)));
	connect(m_webWidget, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(notifyRequestedOpenUrl(QUrl,OpenHints)));
	connect(m_webWidget, SIGNAL(requestedNewWindow(WebWidget*,OpenHints)), this, SLOT(notifyRequestedNewWindow(WebWidget*,OpenHints)));
	connect(m_webWidget, SIGNAL(requestedSearch(QString,QString)), this, SIGNAL(requestedSearch(QString,QString)));
	connect(m_webWidget, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
	connect(m_webWidget, SIGNAL(statusMessageChanged(QString)), this, SIGNAL(statusMessageChanged(QString)));
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

void WebContentsWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_progressBarTimer)
	{
		killTimer(m_progressBarTimer);

		m_progressBarTimer = 0;

		if (!m_progressBarWidget)
		{
			return;
		}

		const QRect geometry = m_webWidget->getProgressBarGeometry();

		if (m_webWidget->isLoading() && geometry.width() > (width() / 2))
		{
			m_progressBarWidget->show();
			m_progressBarWidget->raise();
			m_progressBarWidget->setGeometry(geometry);
		}
		else
		{
			m_progressBarWidget->hide();
		}
	}
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

void WebContentsWidget::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	m_webWidget->setFocus();
}

void WebContentsWidget::resizeEvent(QResizeEvent *event)
{
	if (m_showProgressBar)
	{
		scheduleGeometryUpdate();
	}

	ContentsWidget::resizeEvent(event);
}

void WebContentsWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Browser/ShowDetailedProgressBar"))
	{
		m_showProgressBar = value.toBool();

		if (!m_showProgressBar && m_progressBarWidget)
		{
			m_progressBarWidget->deleteLater();
			m_progressBarWidget = NULL;
		}

		if (m_showProgressBar)
		{
			connect(m_webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(scheduleGeometryUpdate()));
		}
		else
		{
			disconnect(m_webWidget, SIGNAL(progressBarGeometryChanged()), this, SLOT(updateProgressBarWidget()));
		}
	}
	else if (option == QLatin1String("Search/EnableFindInPageAsYouType"))
	{
		if (value.toBool())
		{
			disconnect(m_ui->findLineEdit, SIGNAL(returnPressed()), this, SLOT(updateFind()));
			connect(m_ui->findLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateFind()));
		}
		else
		{
			connect(m_ui->findLineEdit, SIGNAL(returnPressed()), this, SLOT(updateFind()));
			disconnect(m_ui->findLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateFind()));
		}
	}
}

void WebContentsWidget::scheduleGeometryUpdate()
{
	if (m_progressBarWidget && m_progressBarTimer == 0)
	{
		m_progressBarTimer = startTimer(50);
	}
}

void WebContentsWidget::search(const QString &search, const QString &query)
{
	m_webWidget->search(search, query);
}

void WebContentsWidget::print(QPrinter *printer)
{
	m_webWidget->print(printer);
}

void WebContentsWidget::goToHistoryIndex(int index)
{
	m_webWidget->goToHistoryIndex(index);
}

void WebContentsWidget::triggerAction(WindowAction action, bool checked)
{
	if (action == FindAction || action == FindNextAction || action == FindPreviousAction)
	{
		if (!m_ui->findWidget->isVisible())
		{
			switch(SettingsManager::getValue(QLatin1String("Search/ReuseLastQuickFindQuery")).toInt())
			{
	                case 0:
	                    	m_ui->findLineEdit->setText(m_quickFindQuery);
	                    	break;
	                case 1:
	                    	m_ui->findLineEdit->setText(m_webWidget->getSearchInPageQuery());
	                    	break;
	                case 2:
	                default:
	                	break;
			}
			
			if(action == FindAction)
			{
				updateFind();
			}

			m_ui->findWidget->setVisible(true);
		}

		m_ui->findLineEdit->setFocus();
		m_ui->findLineEdit->selectAll();

		if (action != FindAction)
		{
			updateFind(action == FindPreviousAction);
		}
	}
	else
	{
		m_webWidget->triggerAction(action, checked);
	}
}

void WebContentsWidget::setUserAgent(const QString &identifier, const QString &value)
{
	m_webWidget->setUserAgent(identifier, value);
}

void WebContentsWidget::setDefaultTextEncoding(const QString &encoding)
{
	m_webWidget->setDefaultTextEncoding(encoding);
}

void WebContentsWidget::setHistory(const WindowHistoryInformation &history)
{
	m_webWidget->setHistory(history);
}

void WebContentsWidget::setZoom(int zoom)
{
	m_webWidget->setZoom(zoom);
}

void WebContentsWidget::setUrl(const QUrl &url, bool typed)
{
	m_webWidget->setUrl(url, typed);

	if (typed)
	{
		m_webWidget->setFocus();
	}
}

void WebContentsWidget::notifyRequestedOpenUrl(const QUrl &url, OpenHints hints)
{
	if (isPrivate())
	{
		hints |= PrivateOpen;
	}

	emit requestedOpenUrl(url, hints);
}

void WebContentsWidget::notifyRequestedNewWindow(WebWidget *widget, OpenHints hints)
{
	emit requestedNewWindow(new WebContentsWidget(isPrivate(), widget, NULL), hints);
}

void WebContentsWidget::updateFind(bool backwards)
{
	if (sender() && sender()->objectName() == QLatin1String("findPreviousButton"))
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
			palette.setColor(QPalette::Base, QColor(QLatin1String("#CEF6DF")));
		}
		else
		{
			palette.setColor(QPalette::Base, QColor(QLatin1String("#F1E7E4")));
		}
	}

	m_ui->findLineEdit->setPalette(palette);
	m_ui->findNextButton->setEnabled(found);
	m_ui->findPreviousButton->setEnabled(found);

	if (sender() && sender()->objectName() == QLatin1String("caseSensitiveButton"))
	{
		m_webWidget->find(m_ui->findLineEdit->text(), (flags | BackwardFind));
	}

	if (m_ui->findWidget->isVisible() && !isPrivate())
	{
		m_quickFindQuery = m_ui->findLineEdit->text();
		m_webWidget->setSearchInPageQuery(m_ui->findLineEdit->text());
	}

	updateFindHighlight();
}

void WebContentsWidget::updateFindHighlight()
{
	FindFlags flags = NoFlagsFind;

	if (m_ui->highlightButton->isChecked())
	{
		flags |= HighlightAllFind;
	}

	if (m_ui->caseSensitiveButton->isChecked())
	{
		flags |= CaseSensitiveFind;
	}

	m_webWidget->find(QString(), (flags | HighlightAllFind));

	if (m_ui->findWidget->isVisible())
	{
		m_webWidget->find(m_ui->findLineEdit->text(), flags);
	}
}

void WebContentsWidget::setLoading(bool loading)
{
	if (!m_showProgressBar)
	{
		return;
	}

	if (loading && !m_progressBarWidget)
	{
		m_progressBarWidget = new ProgressBarWidget(m_webWidget, this);
	}

	scheduleGeometryUpdate();
}

WebContentsWidget* WebContentsWidget::clone(bool cloneHistory)
{
	if (!canClone())
	{
		return NULL;
	}

	return new WebContentsWidget(m_webWidget->isPrivate(), m_webWidget->clone(cloneHistory), NULL);
}

QAction* WebContentsWidget::getAction(WindowAction action)
{
	return m_webWidget->getAction(action);
}

QUndoStack* WebContentsWidget::getUndoStack()
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

QString WebContentsWidget::getStatusMessage() const
{
	return m_webWidget->getStatusMessage();
}

QLatin1String WebContentsWidget::getType() const
{
	return QLatin1String("web");
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

WindowHistoryInformation WebContentsWidget::getHistory() const
{
	return m_webWidget->getHistory();
}

QPair<QString, QString> WebContentsWidget::getUserAgent() const
{
	return m_webWidget->getUserAgent();
}

int WebContentsWidget::getZoom() const
{
	return m_webWidget->getZoom();
}

bool WebContentsWidget::canClone() const
{
	return true;
}

bool WebContentsWidget::canZoom() const
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

bool WebContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->findWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Escape)
		{
			m_ui->findWidget->hide();
			m_ui->findLineEdit->clear();
		}
	}

	return QObject::eventFilter(object, event);
}

}
