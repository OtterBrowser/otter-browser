/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SourceViewerWebWidget.h"
#include "SourceViewerWidget.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManager.h"
#include "../core/NetworkManagerFactory.h"

#include <QtWidgets/QVBoxLayout>

namespace Otter
{

SourceViewerWebWidget::SourceViewerWebWidget(bool isPrivate, ContentsWidget *parent) : WebWidget(isPrivate, NULL, parent),
	m_sourceViewer(new SourceViewerWidget(this)),
	m_viewSourceReply(NULL),
	m_isLoading(true),
	m_isPrivate(isPrivate)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_sourceViewer);

	connect(m_sourceViewer, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
	connect(m_sourceViewer, SIGNAL(zoomChanged(int)), this, SLOT(handleZoomChange()));
}

void SourceViewerWebWidget::triggerAction(int identifier, bool checked)
{
///TODO
	Q_UNUSED(identifier)
	Q_UNUSED(checked)
}

void SourceViewerWebWidget::print(QPrinter *printer)
{
	m_sourceViewer->print(printer);
}

void SourceViewerWebWidget::pasteText(const QString &text)
{
	Q_UNUSED(text)
}

void SourceViewerWebWidget::viewSourceReplyFinished()
{
	if (m_viewSourceReply)
	{
		setContents(QString(m_viewSourceReply->readAll()));

		m_viewSourceReply->deleteLater();
		m_viewSourceReply = NULL;
	}
}

void SourceViewerWebWidget::clearSelection()
{
	m_sourceViewer->textCursor().clearSelection();
}

void SourceViewerWebWidget::goToHistoryIndex(int index)
{
	Q_UNUSED(index)
}

void SourceViewerWebWidget::handleZoomChange()
{
	SessionsManager::markSessionModified();
}

void SourceViewerWebWidget::setScrollPosition(const QPoint &position)
{
	Q_UNUSED(position)
}

void SourceViewerWebWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)
}

void SourceViewerWebWidget::setZoom(int zoom)
{
	if (zoom != m_sourceViewer->getZoom())
	{
		m_sourceViewer->setZoom(zoom);

		SessionsManager::markSessionModified();

		emit zoomChanged(zoom);
	}
}

void SourceViewerWebWidget::setUrl(const QUrl &url, bool typed)
{
	if (m_viewSourceReply)
	{
		disconnect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));

		m_viewSourceReply->abort();
		m_viewSourceReply->deleteLater();
		m_viewSourceReply = NULL;
	}

	m_url = url;

	if (typed)
	{
		QNetworkRequest request(QUrl(url.toString().mid(12)));
		request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

		m_viewSourceReply = NetworkManagerFactory::getNetworkManager()->get(request);

		connect(m_viewSourceReply, SIGNAL(finished()), this, SLOT(viewSourceReplyFinished()));
	}
}

void SourceViewerWebWidget::setContents(const QString &contents)
{
	m_sourceViewer->setPlainText(contents);

	m_isLoading = false;

	emit loadingChanged(false);
}

WebWidget* SourceViewerWebWidget::clone(bool cloneHistory)
{
	Q_UNUSED(cloneHistory)

	return NULL;
}

Action* SourceViewerWebWidget::getAction(int identifier)
{
	switch (identifier)
	{
		case ActionsManager::FindAction:
		case ActionsManager::FindNextAction:
		case ActionsManager::FindPreviousAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ZoomInAction:
		case ActionsManager::ZoomOutAction:
		case ActionsManager::ZoomOriginalAction:
			break;
		default:
			return NULL;
	}

	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	Action *action = new Action(identifier, this);

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	return action;
}

QString SourceViewerWebWidget::getTitle() const
{
	return tr("Source Viewer");
}

QString SourceViewerWebWidget::getSelectedText() const
{
	return m_sourceViewer->textCursor().selectedText();
}

QUrl SourceViewerWebWidget::getUrl() const
{
	return m_url;
}

QIcon SourceViewerWebWidget::getIcon() const
{
	return HistoryManager::getIcon(getRequestedUrl());
}

QPixmap SourceViewerWebWidget::getThumbnail()
{
	return QPixmap();
}

QPoint SourceViewerWebWidget::getScrollPosition() const
{
	return QPoint();
}

QRect SourceViewerWebWidget::getProgressBarGeometry() const
{
	return QRect();
}

WindowHistoryInformation SourceViewerWebWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.zoom  = getZoom();

	WindowHistoryInformation information;
	information.entries.append(entry);
	information.index = 0;

	return information;
}

int SourceViewerWebWidget::getZoom() const
{
	return m_sourceViewer->getZoom();
}

bool SourceViewerWebWidget::isLoading() const
{
	return m_isLoading;
}

bool SourceViewerWebWidget::isPrivate() const
{
	return m_isPrivate;
}

bool SourceViewerWebWidget::findInPage(const QString &text, WebWidget::FindFlags flags)
{
	return m_sourceViewer->findText(text, flags);
}

}
