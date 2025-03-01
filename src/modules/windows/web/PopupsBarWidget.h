/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_POPUPSBARWIDGET_H
#define OTTER_POPUPSBARWIDGET_H

#include "../../../ui/WebWidget.h"

#if QT_VERSION >= 0x060000
#include <QtGui/QActionGroup>
#else
#include <QtWidgets/QActionGroup>
#endif

namespace Otter
{

namespace Ui
{
	class PopupsBarWidget;
}

class PopupsBarWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit PopupsBarWidget(const QUrl &parentUrl, bool isPrivate, QWidget *parent = nullptr);
	~PopupsBarWidget();

	void addPopup(const QUrl &url);

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void openUrl(QAction *action);
	void populateMenu();

private:
	QActionGroup *m_actionGroup;
	QUrl m_parentUrl;
	QVector<QUrl> m_popupUrls;
	bool m_isPrivate;
	Ui::PopupsBarWidget *m_ui;

signals:
	void requestedClose();
};

}

#endif
