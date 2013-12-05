#include "StatusBarWidget.h"
#include "../core/ActionsManager.h"

namespace Otter
{

StatusBarWidget::StatusBarWidget(QWidget *parent) : QStatusBar(parent),
	m_zoomOutButton(new QToolButton(this)),
	m_zoomInButton(new QToolButton(this)),
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

	m_zoomOutButton->setDefaultAction(ActionsManager::getAction("ZoomOut"));
	m_zoomOutButton->setAutoRaise(true);

	m_zoomInButton->setDefaultAction(ActionsManager::getAction("ZoomIn"));
	m_zoomInButton->setAutoRaise(true);

	addPermanentWidget(m_zoomOutButton);
	addPermanentWidget(m_zoomSlider);
	addPermanentWidget(m_zoomInButton);
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
	m_zoomOutButton->setEnabled(enabled);
	m_zoomInButton->setEnabled(enabled);
	m_zoomSlider->setEnabled(enabled);
}

}
