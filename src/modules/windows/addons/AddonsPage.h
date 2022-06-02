/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ADDONSPAGE_H
#define OTTER_ADDONSPAGE_H

#include "../../../ui/CategoriesTabWidget.h"
#include "../../../ui/WebWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class AddonsPage;
}

class AddonsPage : public CategoryPage, public ActionExecutor
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
	};

	explicit AddonsPage(QWidget *parent);
	~AddonsPage();

	void print(QPrinter *printer);
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	WebWidget::LoadingState getLoadingState() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void timerEvent(QTimerEvent *event) override;
	void changeEvent(QEvent *event) override;
	void addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData = {});
	void load() final override;
	virtual void delayedLoad() = 0;	void markAsFullyLoaded();
	QStandardItemModel* getModel() const;
	virtual QIcon getFallbackIcon() const;
	QModelIndexList getSelectedIndexes() const;
	virtual bool canOpenAddons() const;
	virtual bool canReloadAddons() const;

protected slots:
	virtual void openAddons();
	virtual void reloadAddons();
	virtual void removeAddons() = 0;
	void showContextMenu(const QPoint &position);

private:
	int m_loadingTimer;
	bool m_isLoading;
	Ui::AddonsPage *m_ui;

signals:
	void loadingStateChanged(WebWidget::LoadingState state);
};

}

#endif
