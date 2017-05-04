/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "QtWebKitInspector.h"
#include "QtWebKitWebWidget.h"
#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
#include "../../../../core/ActionsManager.h"
#include "../../../../core/ThemesManager.h"
#endif

namespace Otter
{

QtWebKitInspector::QtWebKitInspector(QtWebKitWebWidget *parent) : QWebInspector(parent)
#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
	, m_widget(parent),
	m_closeButton(new QToolButton(this))
#endif
{
	setMinimumHeight(200);

#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
	m_closeButton->setAutoFillBackground(false);
	m_closeButton->setAutoRaise(true);
	m_closeButton->setIcon(ThemesManager::createIcon(QLatin1String("window-close")));
	m_closeButton->setToolTip(tr("Close"));
	m_closeButton->setFixedSize(16, 16);
	m_closeButton->show();
	m_closeButton->raise();
	m_closeButton->move(QPoint((width() - 19), 3));

	connect(m_closeButton, SIGNAL(clicked()), this, SLOT(hideInspector()));
#endif
}

#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
void QtWebKitInspector::childEvent(QChildEvent *event)
{
	QWebInspector::childEvent(event);

	if (event->type() == QEvent::ChildAdded && event->child()->inherits("QWebView"))
	{
		QWebView *webView(qobject_cast<QWebView*>(event->child()));

		if (webView)
		{
			webView->setContextMenuPolicy(Qt::NoContextMenu);
		}
	}
}

void QtWebKitInspector::showEvent(QShowEvent *event)
{
	QWebInspector::showEvent(event);

	m_closeButton->move(QPoint((width() - 19), 3));
	m_closeButton->raise();
}

void QtWebKitInspector::resizeEvent(QResizeEvent *event)
{
	QWebInspector::resizeEvent(event);

	m_closeButton->move(QPoint((width() - 19), 3));
	m_closeButton->raise();
}

void QtWebKitInspector::hideInspector()
{
	m_widget->triggerAction(ActionsManager::InspectPageAction, {{QLatin1String("isChecked"), false}});
}
#endif

}
