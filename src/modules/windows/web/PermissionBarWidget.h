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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_PERMISSIONBARWIDGET_H
#define OTTER_PERMISSIONBARWIDGET_H

#include "../../../ui/WebWidget.h"

namespace Otter
{

namespace Ui
{
	class PermissionBarWidget;
}

class PermissionBarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PermissionBarWidget(const QString &option, const QUrl &url, QWidget *parent = NULL);
	~PermissionBarWidget();

	QString getOption() const;
	QUrl getUrl() const;

protected:
	void changeEvent(QEvent *event);

protected slots:
	void accepted();
	void rejected();

private:
	QString m_option;
	QUrl m_url;
	Ui::PermissionBarWidget *m_ui;

signals:
	void permissionChanged(WebWidget::PermissionPolicies policies);
};

}

#endif
