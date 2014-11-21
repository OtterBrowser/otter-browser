/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>
* Copyright (C) 2009 by Benjamin C. Meyer <ben@meyerhome.net>
* Copyright (C) 2010-2011 by Matthieu Gicquel <matgic78@gmail.com>
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
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

#include "LoadPluginWidget.h"

#include <QtCore/QUrl>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebView>
#include <QtWidgets/QToolButton>

namespace Otter
{
	LoadPluginWidget::LoadPluginWidget(
			const QString &mimeType,
			const QUrl &url,
			QWidget *parent) :
		QToolButton(parent)
	{
		m_swapping = false;
		setPopupMode(QToolButton::InstantPopup);
		setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		setAutoRaise(false);
		setText(tr("Load plugin (") + QString(mimeType) + tr(")"));
		setToolTip(tr("Load plugin (") + QString(mimeType) + tr(") from ") + url.toDisplayString());
		connect(this, SIGNAL(clicked(bool)), this, SLOT(loadPlugin()));
	}

	void LoadPluginWidget::loadPlugin()
	{
		QWidget *parent = parentWidget();
		QWebView *view = 0;
		while (parent)
		{
			if (QWebView *aView = qobject_cast<QWebView*>(parent))
			{
				view = aView;
				break;
			}
			parent = parent->parentWidget();
		}
		if (!view)
			return;

		QList<QWebFrame*> frames;
		frames.append(view->page()->mainFrame());
		m_swapping = true;
		while (!frames.isEmpty())
		{
			QWebFrame *frame = frames.takeFirst();
			QWebElement docElement = frame->documentElement();

			QWebElementCollection elements;
			elements.append(docElement.findAll(QLatin1String("object")));
			elements.append(docElement.findAll(QLatin1String("embed")));

			Q_FOREACH(QWebElement element, elements)
			{
				if (element.evaluateJavaScript(QLatin1String("this.swapping")).toBool())
				{
					hide();
					QWebElement substitute = element.clone();
					emit signalLoadPlugin(true);
					element.replace(substitute);
					deleteLater();
					return;
				}
			}
			frames += frame->childFrames();
		}
	}

	bool LoadPluginWidget::swapping() const
	{
		return m_swapping;
	}
}
