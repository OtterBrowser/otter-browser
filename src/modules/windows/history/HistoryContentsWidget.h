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
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;

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
