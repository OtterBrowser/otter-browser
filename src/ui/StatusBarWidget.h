#ifndef OTTER_STATUSBARWIDGET_H
#define OTTER_STATUSBARWIDGET_H

#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>

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
	QSlider *m_zoomSlider;

signals:
	void requestedZoomChange(int zoom);
};

}

#endif
