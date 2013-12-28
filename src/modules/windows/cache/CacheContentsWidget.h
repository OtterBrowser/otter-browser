#ifndef OTTER_CacheContentsWidget_H
#define OTTER_CacheContentsWidget_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class CacheContentsWidget;
}

class Window;

class CacheContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit CacheContentsWidget(Window *window);
	~CacheContentsWidget();

	void print(QPrinter *printer);
	QString getTitle() const;
	QLatin1String getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;

protected:
	void changeEvent(QEvent *event);
	QStandardItem* findDomain(const QString &domain);
	QStandardItem* findEntry(const QUrl &entry);
	QUrl getEntry(const QModelIndex &index) const;

protected slots:
	void populateCache();
	void filterCache(const QString &filter);
	void clearEntries();
	void addEntry(const QUrl &entry);
	void removeEntry(const QUrl &entry);
	void removeEntry();
	void removeDomainEntries();
	void removeDomainEntriesOrEntry();
	void openEntry(const QModelIndex &index = QModelIndex());
	void copyEntryLink();
	void showContextMenu(const QPoint &point);
	void updateActions();

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::CacheContentsWidget *m_ui;
};

}

#endif
