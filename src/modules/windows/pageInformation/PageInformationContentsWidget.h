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

#ifndef OTTER_PAGEINFORMATIONCONTENTSWIDGET_H
#define OTTER_PAGEINFORMATIONCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItem>

namespace Otter
{

namespace Ui
{
	class PageInformationContentsWidget;
}

class Window;

class PageInformationContentsWidget final : public ActiveWindowObserverContentsWidget
{
	Q_OBJECT

public:
	enum SectionName
	{
		UnknownSection = 0,
		GeneralSection,
		HeadersSection,
		MetaSection,
		PermissionsSection,
		SecuritySection
	};

	explicit PageInformationContentsWidget(const QVariantMap &parameters, QWidget *parent);
	~PageInformationContentsWidget();

	QString getTitle() const override;
	QLatin1String getType() const override;
	QIcon getIcon() const override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;
	void addEntry(QStandardItem *parent, const QString &label, const QString &value);
	void updateSections();

protected slots:
	void handleWatchedDataChanged(WebWidget::ChangeWatcher watcher);
	void showContextMenu(const QPoint &position);

private:
	Ui::PageInformationContentsWidget *m_ui;
};

}

#endif
