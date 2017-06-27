/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreviewWidget.h"
#include "../core/SettingsManager.h"

#include <QtGui/QGuiApplication>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

PreviewWidget::PreviewWidget(QWidget *parent) : QFrame(parent),
	m_textLabel(new QLabel(this)),
	m_pixmapLabel(new QLabel(this)),
	m_moveAnimation(nullptr)
{
	QVBoxLayout *layout(new QVBoxLayout(this));
	layout->addWidget(m_pixmapLabel);
	layout->addWidget(m_textLabel);

	QPalette palette;
	palette.setColor(backgroundRole(), palette.color(QPalette::ToolTipBase));
	palette.setColor(QPalette::Base, palette.color(QPalette::ToolTipBase));
	palette.setColor(foregroundRole(), palette.color(QPalette::ToolTipText));
	palette.setColor(QPalette::Text, palette.color(QPalette::ToolTipText));

	setPalette(palette);
	setLayout(layout);
	setWindowFlags(windowFlags() | Qt::ToolTip);
	setObjectName(QLatin1String("previewWidget"));
	setStyleSheet(QLatin1String("#previewWidget {border:1px solid #B3B3B3;border-radius:4px;}"));

	m_textLabel->setFixedWidth(260);
	m_textLabel->setAlignment(Qt::AlignCenter);
	m_textLabel->setTextFormat(Qt::PlainText);
	m_textLabel->setPalette(palette);

	m_pixmapLabel->setFixedWidth(260);
	m_pixmapLabel->setAlignment(Qt::AlignCenter);
	m_pixmapLabel->setPalette(palette);
	m_pixmapLabel->setStyleSheet(QLatin1String("border:1px solid gray;"));
}

void PreviewWidget::setPosition(const QPoint &position)
{
	if (!m_moveAnimation)
	{
		m_moveAnimation = new QPropertyAnimation(this, QStringLiteral("pos").toLatin1());
		m_moveAnimation->setDuration(SettingsManager::getOption(SettingsManager::TabBar_PreviewsAnimationDurationOption).toInt());
	}

	if (position != m_moveAnimation->endValue().toPoint())
	{
		m_moveAnimation->stop();
	}

	m_moveAnimation->setStartValue(pos());
	m_moveAnimation->setEndValue(position);
	m_moveAnimation->start();
}

void PreviewWidget::setPreview(const QString &text, const QPixmap &pixmap, bool showFullText)
{
	if (pixmap.isNull())
	{
		m_pixmapLabel->hide();
	}
	else
	{
		m_pixmapLabel->setPixmap(pixmap);
		m_pixmapLabel->show();
	}

	const int width(m_textLabel->fontMetrics().width(text) + 8);

	m_textLabel->setFixedWidth(((showFullText || pixmap.isNull()) && width < 260) ? width : 260);
	m_textLabel->setText(showFullText ? text : m_textLabel->fontMetrics().elidedText(text, (QGuiApplication::isLeftToRight() ? Qt::ElideRight : Qt::ElideLeft), m_textLabel->width()));
	m_textLabel->setWordWrap(showFullText);

	adjustSize();
}

}
