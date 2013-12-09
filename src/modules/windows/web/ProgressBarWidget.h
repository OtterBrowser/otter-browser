#ifndef OTTER_PROGRESSBARWIDGET_H
#define OTTER_PROGRESSBARWIDGET_H

#include <QtCore/QTime>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>

namespace Otter
{

class WebWidget;

class ProgressBarWidget : public QFrame
{
	Q_OBJECT

public:
	explicit ProgressBarWidget(WebWidget *webWidget, QWidget *parent = NULL);

protected:
	void timerEvent(QTimerEvent *event);

protected slots:
	void updateLoadStatus(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
	void setLoading(bool loading);

private:
	WebWidget *m_webWidget;
	QProgressBar *m_progressBar;
	QLabel *m_elementsLabel;
	QLabel *m_totalLabel;
	QLabel *m_speedLabel;
	QLabel *m_elapsedLabel;
	QLabel *m_messageLabel;
	QTime *m_time;
};

}

#endif
