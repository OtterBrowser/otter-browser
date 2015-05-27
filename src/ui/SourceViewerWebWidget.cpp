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

#include <QtWidgets/QVBoxLayout>

namespace Otter
{

SourceViewerWebWidget::SourceViewerWebWidget(bool isPrivate, ContentsWidget *parent) : WebWidget(isPrivate, NULL, parent),
	m_sourceViewer(new SourceViewerWidget(this)),
	m_isLoading(true),
	m_isPrivate(isPrivate)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_sourceViewer);
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

void SourceViewerWebWidget::clearSelection()
{
	m_sourceViewer->textCursor().clearSelection();
}

void SourceViewerWebWidget::goToHistoryIndex(int index)
{
	Q_UNUSED(index)
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
	m_sourceViewer->setZoom(zoom);
}

void SourceViewerWebWidget::setUrl(const QUrl &url, bool typed)
{
	Q_UNUSED(url)
	Q_UNUSED(typed)
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
	Q_UNUSED(identifier)

	return NULL;
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
	return getRequestedUrl();
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
	entry.url = getRequestedUrl().toString();
	entry.url = getTitle();
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
///TODO
	Q_UNUSED(text)
	Q_UNUSED(flags)
}

}
