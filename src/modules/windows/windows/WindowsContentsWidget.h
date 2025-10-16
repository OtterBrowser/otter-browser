/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WINDOWSCONTENTSWIDGET_H
#define OTTER_WINDOWSCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ItemDelegate.h"

namespace Otter
{

namespace Ui
{
	class WindowsContentsWidget;
}

class Window;


class EntryItemDelegate final : public ItemDelegate
{
public:
	explicit EntryItemDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
	int calculateDecorationWidth(QStyleOptionViewItem *option, const QModelIndex &index) const;

private:
	static int m_decorationSize;
};

class WindowsContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	explicit WindowsContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent);
	~WindowsContentsWidget();

	void print(QPrinter *printer) override;
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void activateWindow(const QModelIndex &index);
	void showContextMenu(const QPoint &position);

private:
	Ui::WindowsContentsWidget *m_ui;
};

}

#endif
