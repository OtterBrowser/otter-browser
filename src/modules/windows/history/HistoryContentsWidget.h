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
	QStandardItem* findEntry(qint64 entry);
	qint64 getEntry(const QModelIndex &index);

protected slots:
	void filterHistory(const QString &filter);
	void populateEntries();
	void clearEntries();
	void addEntry(qint64 entry);
	void addEntry(const HistoryEntry &entry, bool sort = true);
	void updateEntry(qint64 entry);
	void removeEntry(qint64 entry);
	void removeEntry();
	void removeDomainEntries();
	void openEntry(const QModelIndex &index = QModelIndex());
	void bookmarkEntry();
	void copyEntryLink();
	void showContextMenu(const QPoint &point);

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::HistoryContentsWidget *m_ui;
};

}

#endif
