/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TABHISTORYCONTENTSWIDGET_H
#define OTTER_TABHISTORYCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

namespace Otter
{

namespace Ui
{
	class TabHistoryContentsWidget;
}

class Window;

class TabHistoryContentsWidget final : public ActiveWindowObserverContentsWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		TitleRole = Qt::DisplayRole,
		TimeVisitedRole = Qt::UserRole,
		UrlRole = Qt::StatusTipRole
	};

	explicit TabHistoryContentsWidget(const QVariantMap &parameters, QWidget *parent);
	~TabHistoryContentsWidget();

	QString getTitle() const override;
	QLatin1String getType() const override;
	QIcon getIcon() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void updateHistory();
	void showContextMenu(const QPoint &position);

private:
	Ui::TabHistoryContentsWidget *m_ui;
};

}

#endif
