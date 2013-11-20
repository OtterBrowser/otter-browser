#include "ProgressBarWidget.h"
#include "../backends/web/WebWidget.h"

#include <QtWidgets/QHBoxLayout>

namespace Otter
{

ProgressBarWidget::ProgressBarWidget(WebWidget *webWidget, QWidget *parent) : QWidget(parent),
	m_webWidget(webWidget),
	m_progressBar(new QProgressBar(this)),
	m_elementsLabel(new QLabel(this)),
	m_totalLabel(new QLabel(this)),
	m_speedLabel(new QLabel(this)),
	m_elapsedLabel(new QLabel(this)),
	m_messageLabel(new QLabel(this)),
	m_time(NULL)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(m_progressBar);
	layout->addWidget(m_elementsLabel);
	layout->addWidget(m_totalLabel);
	layout->addWidget(m_speedLabel);
	layout->addWidget(m_elapsedLabel);
	layout->addWidget(m_messageLabel);
	layout->setContentsMargins(0, 0, 0, 0);

	m_progressBar->setFixedWidth(120);
	m_progressBar->setFormat(tr("Loading: %p%"));
	m_elementsLabel->setFixedWidth(120);
	m_totalLabel->setFixedWidth(120);
	m_speedLabel->setFixedWidth(120);
	m_elapsedLabel->setFixedWidth(120);

	setAutoFillBackground(true);
	setLoading(webWidget->isLoading());

	connect(webWidget, SIGNAL(loadProgress(int)), m_progressBar, SLOT(setValue(int)));
	connect(webWidget, SIGNAL(loadingChanged(bool)), this, SLOT(setLoading(bool)));
}

void ProgressBarWidget::timerEvent(QTimerEvent *event)
{
	if (m_time)
	{
		QString string;
		int seconds = (m_time->elapsed() / 1000);
		int minutes = (seconds / 60);

		seconds = (seconds - (minutes * 60));

		m_elapsedLabel->setText(tr("Time: %1").arg(QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'))));
	}
	else
	{
		m_elapsedLabel->setText(QString());
	}


	if (!m_webWidget->isLoading())
	{
		killTimer(event->timerId());
	}
}

void ProgressBarWidget::setLoading(bool loading)
{
	if (loading)
	{
		show();

		m_time = new QTime();
		m_time->start();

		startTimer(1000);
	}
	else
	{
		if (m_time)
		{
			delete m_time;

			m_time = NULL;
		}

		hide();
	}
}

}
