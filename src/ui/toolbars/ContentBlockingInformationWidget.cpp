/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingInformationWidget.h"
#include "../ContentsWidget.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/ThemesManager.h"

#include <QtWidgets/QStyleOptionToolButton>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ContentBlockingInformationWidget::ContentBlockingInformationWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_window(window),
	m_amount(0)
{
	setIcon(ThemesManager::getIcon(QLatin1String("content-blocking")));
	setWindow(window);

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
}

void ContentBlockingInformationWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionToolButton option;

	initStyleOption(&option);

	if (m_icon.isNull())
	{
		m_icon = getOptions().value(QLatin1String("icon")).value<QIcon>();

		if (m_icon.isNull())
		{
			m_icon = ThemesManager::getIcon(QLatin1String("content-blocking"));
		}

		const int iconSize(option.iconSize.width());
		const int fontSize(qMax((iconSize / 2), 12));
		QFont font(option.font);
		font.setBold(true);
		font.setPixelSize(fontSize);

		const QString text(QString::number(m_amount));
		const qreal textWidth(QFontMetricsF(font).width(text));

		font.setPixelSize(fontSize * 0.8);

		QRectF rectangle((iconSize - textWidth), (iconSize - fontSize), textWidth, fontSize);
		QPixmap pixmap(m_icon.pixmap(option.iconSize));
		QPainter iconPainter(&pixmap);
		iconPainter.fillRect(rectangle, Qt::red);
		iconPainter.setFont(font);
		iconPainter.setPen(QColor(255, 255, 255, 230));
		iconPainter.drawText(rectangle, Qt::AlignCenter, text);

		m_icon = QIcon(pixmap);
	}

	option.icon = m_icon;
	option.text = option.fontMetrics.elidedText(option.text, Qt::ElideRight, (option.rect.width() - (option.fontMetrics.width(QLatin1Char(' ')) * 2) - ((toolButtonStyle() == Qt::ToolButtonTextBesideIcon) ? option.iconSize.width() : 0)));

	painter.drawComplexControl(QStyle::CC_ToolButton, option);
}

void ContentBlockingInformationWidget::resizeEvent(QResizeEvent *event)
{
	m_icon = QIcon();

	ToolButtonWidget::resizeEvent(event);
}

void ContentBlockingInformationWidget::clear()
{
	m_amount = 0;

	updateState();
}

void ContentBlockingInformationWidget::handleRequest(const NetworkManager::ResourceInformation &request)
{
	Q_UNUSED(request)

	++m_amount;

	updateState();
}

void ContentBlockingInformationWidget::updateState()
{
	m_icon = QIcon();

	QString text(tr("Blocked Elements: %1"));

	if (isCustomized())
	{
		const QVariantMap options(getOptions());

		if (options.contains(QLatin1String("text")))
		{
			text = options[QLatin1String("text")].toString();
		}
	}

	text = text.arg(m_amount);

	setText(text);
	setToolTip(text);
}

void ContentBlockingInformationWidget::setWindow(Window *window)
{
	if (m_window)
	{
		disconnect(m_window->getContentsWidget(), SIGNAL(aboutToNavigate()), this, SLOT(clear()));
		disconnect(m_window->getContentsWidget(), SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SLOT(handleRequest(NetworkManager::ResourceInformation)));
	}

	m_window = window;
	m_amount = 0;

	if (window)
	{
		m_amount = window->getContentsWidget()->getBlockedRequests().count();

		connect(m_window->getContentsWidget(), SIGNAL(aboutToNavigate()), this, SLOT(clear()));
		connect(m_window->getContentsWidget(), SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SLOT(handleRequest(NetworkManager::ResourceInformation)));
	}

	updateState();
}

}
