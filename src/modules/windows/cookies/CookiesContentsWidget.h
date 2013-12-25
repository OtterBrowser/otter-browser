#ifndef OTTER_COOKIESCONTENTSWIDGET_H
#define OTTER_COOKIESCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkCookie>

namespace Otter
{

namespace Ui
{
	class CookiesContentsWidget;
}

class Window;

class CookiesContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit CookiesContentsWidget(Window *window);
	~CookiesContentsWidget();

	void print(QPrinter *printer);
	QAction* getAction(WindowAction action);
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);

protected:
	void changeEvent(QEvent *event);
	QStandardItem* findDomain(const QString &domain);
	QNetworkCookie getCookie(const QModelIndex &index) const;

protected slots:
	void triggerAction();
	void populateCookies();
	void filterCookies(const QString &filter);
	void addCookie(const QNetworkCookie &cookie);
	void removeCookie(const QNetworkCookie &cookie);
	void removeCookies();
	void removeDomainCookies();
	void showContextMenu(const QPoint &point);
	void updateActions();

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::CookiesContentsWidget *m_ui;
};

}

#endif
