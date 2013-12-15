#ifndef OTTER_ADDRESSWIDGET_H
#define OTTER_ADDRESSWIDGET_H

#include <QtCore/QUrl>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

namespace Otter
{

class Window;

class AddressWidget : public QLineEdit
{
	Q_OBJECT

public:
	explicit AddressWidget(QWidget *parent = NULL);

	void setWindow(Window *window);
	QUrl getUrl() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void setUrl(const QUrl &url);

protected:
	void resizeEvent(QResizeEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void notifyRequestedLoadUrl();
	void updateBookmark();
	void setIcon(const QIcon &icon);

private:
	Window *m_window;
	QLabel *m_bookmarkLabel;
	QLabel *m_urlIconLabel;

signals:
	void requestedLoadUrl(QUrl url);
	void requestedSearch(QString query, QString engine);
};

}

#endif
