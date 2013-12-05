#include "StatusBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtWidgets/QToolButton>

namespace Otter
{

StatusBarWidget::StatusBarWidget(QWidget *parent) : QStatusBar(parent),
	m_zoomSlider(NULL)
{
}

void StatusBarWidget::setup()
{
	m_zoomSlider = new QSlider(this);
	m_zoomSlider->setRange(10, 250);
	m_zoomSlider->setTracking(true);;
	m_zoomSlider->setOrientation(Qt::Horizontal);
	m_zoomSlider->setMaximumWidth(100);

	QToolButton *zoomOutButton = new QToolButton(this);
	zoomOutButton->setDefaultAction(ActionsManager::getAction("ZoomOut"));
	zoomOutButton->setAutoRaise(true);

	QToolButton *zoomInButton = new QToolButton(this);
	zoomInButton->setDefaultAction(ActionsManager::getAction("ZoomIn"));
	zoomInButton->setAutoRaise(true);

	addPermanentWidget(zoomOutButton);
	addPermanentWidget(m_zoomSlider);
	addPermanentWidget(zoomInButton);
	setZoom(100);

	connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SIGNAL(requestedZoomChange(int)));
}

void StatusBarWidget::setZoom(int zoom)
{
	m_zoomSlider->setValue(zoom);
	m_zoomSlider->setToolTip(tr("Zoom %1%").arg(zoom));
}

void StatusBarWidget::setZoomEnabled(bool enabled)
{
	m_zoomSlider->setEnabled(enabled);
}

}
