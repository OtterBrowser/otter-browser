#ifndef OTTER_CONFIGURATIONCONTENTSWIDGET_H
#define OTTER_CONFIGURATIONCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>
#include <QtNetwork/QNetworkCookie>

namespace Otter
{

namespace Ui
{
	class ConfigurationContentsWidget;
}

class Window;

class ConfigurationContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit ConfigurationContentsWidget(Window *window);
	~ConfigurationContentsWidget();

	void print(QPrinter *printer);
	ContentsWidget* clone(Window *window = NULL);
	QAction* getAction(WindowAction action);
	QUndoStack* getUndoStack();
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	HistoryInformation getHistory() const;
	int getZoom() const;
	bool canZoom() const;
	bool isClonable() const;
	bool isLoading() const;
	bool isPrivate() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);
	void setHistory(const HistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void filterConfiguration(const QString &filter);
	void currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
	void optionChanged(const QString &option, const QVariant &value);

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::ConfigurationContentsWidget *m_ui;
};

}

#endif
