#ifndef OTTER_STATUSBARWIDGET_H
#define OTTER_STATUSBARWIDGET_H

#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolButton>

namespace Otter
{

class StatusBarWidget : public QStatusBar
{
	Q_OBJECT

public:
	explicit StatusBarWidget(QWidget *parent = NULL);

	void setup();

public slots:
	void setZoom(int zoom);
	void setZoomEnabled(bool enabled);

private:
	QToolButton *m_zoomOutButton;
	QToolButton *m_zoomInButton;
	QSlider *m_zoomSlider;

signals:
	void requestedZoomChange(int zoom);
};

}

#endif
