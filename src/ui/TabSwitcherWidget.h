/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>

namespace Otter
{

class Window;
class WindowsManager;

class TabSwitcherWidget : public QWidget
{
	Q_OBJECT

public:
	enum SwitcherReason
	{
		ActionReason = 0,
		KeyboardReason = 1,
		WheelReason = 2
	};

	explicit TabSwitcherWidget(WindowsManager *manager, QWidget *parent = nullptr);

	void show(SwitcherReason reason);
	void accept();
	void selectTab(bool next);
	SwitcherReason getReason() const;
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	QStandardItem* createRow(Window *window) const;
	int findRow(qint64 identifier) const;

protected slots:
	void currentTabChanged(const QModelIndex &index);
	void tabAdded(qint64 identifier);
	void tabRemoved(qint64 identifier);
	void setTitle(const QString &title);
	void setIcon(const QIcon &icon);

private:
	WindowsManager *m_windowsManager;
	QStandardItemModel *m_model;
	QFrame *m_frame;
	ItemViewWidget *m_tabsView;
	QLabel *m_previewLabel;
	QMovie *m_loadingMovie;
	SwitcherReason m_reason;
};

}

#endif
