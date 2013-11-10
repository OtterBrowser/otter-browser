#ifndef OTTER_WINDOW_H
#define OTTER_WINDOW_H

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class Window;
}

class Window : public QWidget
{
	Q_OBJECT

public:
	explicit Window(QWidget *parent = NULL);
	~Window();

	virtual QString getTitle() const;
	virtual QUrl getUrl() const;
	virtual QIcon getIcon() const;

signals:
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void loadUrl();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();

private:
	Ui::Window *m_ui;
};

}

#endif
