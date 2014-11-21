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

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebView>

namespace Otter
{

LoadPluginWidget::LoadPluginWidget(QWebPage *page, const QString &mimeType, const QUrl &url, QWidget *parent) : QToolButton(parent),
	m_page(page),
	m_isSwapping(false)
{
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setText(tr("Load plugin (%1)").arg(mimeType));
	setToolTip(tr("Load plugin (%1) from %2").arg(mimeType).arg(url.toDisplayString()));

	connect(this, SIGNAL(clicked()), this, SLOT(loadPlugin()));
}

void LoadPluginWidget::loadPlugin()
{
	QWidget *parent = m_page->view();
	QWebView *view = NULL;

	while (parent)
	{
		view = qobject_cast<QWebView*>(parent);

		if (view)
		{
			break;
		}

		parent = parent->parentWidget();
	}

	if (!view)
	{
		return;
	}

	QList<QWebFrame*> frames;
	frames.append(view->page()->mainFrame());

	m_isSwapping = true;

	while (!frames.isEmpty())
	{
		QWebFrame *frame = frames.takeFirst();
		QWebElementCollection elements;
		elements.append(frame->documentElement().findAll(QLatin1String("object")));
		elements.append(frame->documentElement().findAll(QLatin1String("embed")));

		for (int i = 0; i < elements.count(); ++i)
		{
			if (elements.at(i).evaluateJavaScript(QLatin1String("this.isSwapping")).toBool())
			{
				hide();

				const QWebElement substitute = elements.at(i).clone();

				emit pluginLoaded();

				elements.at(i).replace(substitute);

				deleteLater();

				return;
			}
		}

		frames.append(frame->childFrames());
	}
}

bool LoadPluginWidget::isSwapping() const
{
	return m_isSwapping;
}

}
