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

#include <QtGui/QMouseEvent>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebView>

namespace Otter
{

LoadPluginWidget::LoadPluginWidget(QWebPage *page, const QString &mimeType, const QUrl &url, QWidget *parent) : QWidget(parent),
	m_page(page),
	m_isHovered(false),
	m_isSwapping(false)
{
	setToolTip(tr("Click to load content (%1) handled by plugin from: %2").arg(mimeType).arg(url.toDisplayString()));
}

void LoadPluginWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	int size = 0;

	if (width() > height())
	{
		size = height();
	}
	else
	{
		size = width();
	}

	const QRect rectangle(((width() - size) / 2), ((height() - size) / 2), size, size);

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);
	painter.fillRect(rect(), Qt::transparent);

	if (m_isHovered)
	{
		painter.setPen(QColor(215, 215, 215));
		painter.drawRect(rect());
	}

	painter.setBrush(QColor(215, 215, 215));
	painter.setPen(Qt::transparent);
	painter.drawEllipse(rectangle);

	QPainterPath path;
	path.moveTo(rectangle.topLeft() + QPoint((size / 3), (size / 4)));
	path.lineTo(rectangle.topLeft() + QPoint(((size / 4) * 3), (size / 2)));
	path.lineTo(rectangle.topLeft() + QPoint((size / 3), ((size / 4) * 3)));
	path.lineTo(rectangle.topLeft() + QPoint((size / 3), (size / 4)));

	painter.fillPath(path, QColor(255, 255, 255));
}

void LoadPluginWidget::enterEvent(QEvent *event)
{
	QWidget::enterEvent(event);

	m_isHovered = true;

	update();
}

void LoadPluginWidget::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	m_isHovered = false;

	update();
}

void LoadPluginWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		loadPlugin();

		event->accept();
	}
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
