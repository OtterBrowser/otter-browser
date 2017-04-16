/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_HISTORYCONTENTSWIDGET_H
#define OTTER_HISTORYCONTENTSWIDGET_H

#include "../../../core/HistoryManager.h"
#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class HistoryContentsWidget;
}

class Window;

class HistoryContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	explicit HistoryContentsWidget(const QVariantMap &parameters, Window *window);
	~HistoryContentsWidget();

	void print(QPrinter *printer) override;
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	WebWidget::LoadingState getLoadingState() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap()) override;

protected:
	void changeEvent(QEvent *event) override;
	QStandardItem* findEntry(quint64 identifier);
	quint64 getEntry(const QModelIndex &index) const;

protected slots:
	void populateEntries();
	void addEntry(HistoryEntryItem *entry);
	void modifyEntry(HistoryEntryItem *entry);
	void removeEntry(HistoryEntryItem *entry);
	void removeEntry();
	void removeDomainEntries();
	void openEntry(const QModelIndex &index = QModelIndex());
	void bookmarkEntry();
	void copyEntryLink();
	void showContextMenu(const QPoint &position);

private:
	QStandardItemModel *m_model;
	bool m_isLoading;
	Ui::HistoryContentsWidget *m_ui;
};

}

#endif
