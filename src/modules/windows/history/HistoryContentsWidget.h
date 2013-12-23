#ifndef OTTER_HISTORYCONTENTSWIDGET_H
#define OTTER_HISTORYCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class HistoryContentsWidget;
}

struct HistoryEntry;

class Window;

class HistoryContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit HistoryContentsWidget(Window *window);
	~HistoryContentsWidget();

	void print(QPrinter *printer);
	ContentsWidget* clone(Window *window = NULL);
	QAction* getAction(WindowAction action);
	QUndoStack* getUndoStack();
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	WindowHistoryInformation getHistory() const;
	int getZoom() const;
	bool canZoom() const;
	bool isClonable() const;
	bool isLoading() const;
	bool isPrivate() const;

public slots:
	void triggerAction(WindowAction action, bool checked = false);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);
	void updateGroups();
	QStandardItem* findEntry(qint64 entry);

protected slots:
	void filterHistory(const QString &filter);
	void clearEntries();
	void addEntry(qint64 entry);
	void addEntry(const HistoryEntry &entry, bool sort = true);
	void updateEntry(qint64 entry);
	void removeEntry(qint64 entry);
	void openEntry(const QModelIndex &index);

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::HistoryContentsWidget *m_ui;
};

}

#endif
