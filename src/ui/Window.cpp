#include "Window.h"
#include "ui_Window.h"

namespace Otter
{

Window::Window(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::Window)
{
	m_ui->setupUi(this);

	connect(m_ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(loadUrl());
	connect(m_ui->webView, SIGNAL(titleChanged(const QString)), this, SIGNAL(titleChanged(const QString)));
	connect(m_ui->webView, SIGNAL(urlChanged(const QUrl)), this, SLOT(notifyUrlChanged(const QUrl)));
	connect(m_ui->webView, SIGNAL(iconChanged()), this, SLOT(notifyIconChanged()));
}

Window::~Window()
{
	delete m_ui;
}

QString Window::getTitle() const
{
	return m_ui->webView->title();
}

QUrl Window::getUrl() const
{
	return m_ui->webView->url();
}

QIcon Window::getIcon() const
{
	return m_ui->webView->icon();
}

void Window::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void Window::loadUrl()
{
	m_ui->webView->setUrl(QUrl(m_ui->lineEdit->text()));
}

void Window::notifyUrlChanged(const QUrl &url)
{
	m_ui->lineEdit->setText(url.toString());

	emit urlChanged(url);
}

void Window::notifyIconChanged()
{
	emit iconChanged(m_ui->webView->icon());
}

}
