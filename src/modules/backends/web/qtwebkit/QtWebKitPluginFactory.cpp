/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Martin Rejda <rejdi@otter.ksp.sk>
* Copyright (C) 2014 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebKitPluginFactory.h"
#include "QtWebKitWebWidget.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace Otter
{

QtWebKitPluginWidget::QtWebKitPluginWidget(const QString &mimeType, const QUrl &url, QWidget *parent) : QWidget(parent),
	m_mimeType(mimeType),
	m_url(url.toDisplayString()),
	m_isHovered(false)
{
	setToolTip(tr("Click to load content (%1) handled by plugin from: %2").arg(mimeType, url.toDisplayString()));
}

void QtWebKitPluginWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		setToolTip(tr("Click to load content (%1) handled by plugin from: %2").arg(m_mimeType, m_url));
	}
}

void QtWebKitPluginWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	int size(0);

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

void QtWebKitPluginWidget::enterEvent(QEvent *event)
{
	QWidget::enterEvent(event);

	m_isHovered = true;

	update();
}

void QtWebKitPluginWidget::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	m_isHovered = false;

	update();
}

QtWebKitPluginFactory::QtWebKitPluginFactory(QtWebKitWebWidget *parent) : QWebPluginFactory(parent),
	m_widget(parent)
{
}

QObject* QtWebKitPluginFactory::create(const QString &mimeType, const QUrl &url, const QStringList &argumentNames, const QStringList &argumentValues) const
{
	const bool isActivatingPlugin(argumentNames.contains(QLatin1String("data-otter-browser")) && argumentValues.value(argumentNames.indexOf(QLatin1String("data-otter-browser"))) == m_widget->getPluginToken());

	if (m_widget->canLoadPlugins() || isActivatingPlugin)
	{
		if (isActivatingPlugin)
		{
			m_widget->clearPluginToken();
		}

		return nullptr;
	}

	return new QtWebKitPluginWidget(mimeType, url, m_widget);
}

QList<QWebPluginFactory::Plugin> QtWebKitPluginFactory::plugins() const
{
	return {};
}

}
