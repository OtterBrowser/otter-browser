/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TABSWITCHERWIDGET_H
#define OTTER_TABSWITCHERWIDGET_H

#include "ItemViewWidget.h"
#include "WebWidget.h"

#include <QtWidgets/QLabel>

namespace Otter
{

class Animation;
class Window;

class TabSwitcherWidget final : public QWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		OrderRole
	};

	enum SwitcherReason
	{
		ActionReason = 0,
		KeyboardReason,
		WheelReason
	};

	explicit TabSwitcherWidget(MainWindow *parent);

	void show(SwitcherReason reason);
	void accept();
	void selectTab(bool next);
	SwitcherReason getReason() const;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	QStandardItem* createRow(Window *window, const QVariant &index) const;
	int findRow(quint64 identifier) const;

protected slots:
	void handleIndexClicked(const QModelIndex &index);
	void handleCurrentTabChanged(const QModelIndex &index);
	void handleWindowAdded(quint64 identifier);
	void handleWindowRemoved(quint64 identifier);
	void setTitle(const QString &title);
	void setIcon(const QIcon &icon);
	void setLoadingState(WebWidget::LoadingState state);

private:
	MainWindow *m_mainWindow;
	QStandardItemModel *m_model;
	ItemViewWidget *m_tabsView;
	QLabel *m_previewLabel;
	Animation *m_spinnerAnimation;
	SwitcherReason m_reason;
	bool m_isIgnoringMinimizedTabs;
};

}

#endif
