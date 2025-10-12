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

#ifndef OTTER_LINKSCONTENTSWIDGET_H
#define OTTER_LINKSCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItem>

namespace Otter
{

namespace Ui
{
	class LinksContentsWidget;
}

class Window;

class LinksContentsWidget final : public ActiveWindowObserverContentsWidget
{
	Q_OBJECT

public:
	explicit LinksContentsWidget(const QVariantMap &parameters, QWidget *parent);
	~LinksContentsWidget();

	QString getTitle() const override;
	QLatin1String getType() const override;
	QIcon getIcon() const override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;
	void addLink(const QString &title, const QUrl &url);
	void updateLinks();

protected slots:
	void openLink();
	void handleWatchedDataChanged(WebWidget::ChangeWatcher watcher);
	void showContextMenu(const QPoint &position);

private:
	bool m_isLocked;
	Ui::LinksContentsWidget *m_ui;
};

}

#endif
