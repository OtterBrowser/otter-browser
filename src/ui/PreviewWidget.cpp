#include "PreviewWidget.h"

#include <QtGui/QGuiApplication>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

PreviewWidget::PreviewWidget(QWidget *parent) : QWidget(parent),
	m_textLabel(new QLabel(this)),
	m_pixmapLabel(new QLabel(this))
{
	QVBoxLayout *layout = new QVBoxLayout(this);
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

	m_textLabel->setFixedWidth(260);
	m_textLabel->setAlignment(Qt::AlignCenter);

	m_pixmapLabel->setFixedWidth(260);
	m_pixmapLabel->setAlignment(Qt::AlignCenter);
}

void PreviewWidget::setPreview(const QString &text, const QPixmap &pixmap)
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

	m_textLabel->setText(m_textLabel->fontMetrics().elidedText(text, (QGuiApplication::isLeftToRight() ? Qt::ElideRight : Qt::ElideLeft), m_textLabel->width()));
}

}
