/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

namespace Otter
{

QtWebKitInspector::QtWebKitInspector(QtWebKitWebWidget *parent) : QWebInspector(parent)
{
	setMinimumHeight(200);
}

void QtWebKitInspector::childEvent(QChildEvent *event)
{
	QWebInspector::childEvent(event);

	if (event->type() == QEvent::ChildAdded && event->child()->inherits("QWebView"))
	{
		QWebView *webView(qobject_cast<QWebView*>(event->child()));

		if (webView)
		{
			webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
		}
	}
}

}
