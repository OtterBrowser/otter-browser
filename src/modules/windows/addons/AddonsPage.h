/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

class ItemViewWidget;

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

	explicit AddonsPage(bool needsDetails, QWidget *parent);
	~AddonsPage();

	void print(QPrinter *printer);
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	WebWidget::LoadingState getLoadingState() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	virtual void addAddon() = 0;

protected:
	struct ModelColumn
	{
		QString label;
		int width = 0;
	};

	struct DetailsEntry
	{
		QString label;
		QString value;
		bool isUrl = false;
	};

	void timerEvent(QTimerEvent *event) override;
	void changeEvent(QEvent *event) override;
	virtual void addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData = {});
	virtual void updateAddonEntry(Addon *addon);
	void load() final override;
	virtual void delayedLoad() = 0;	void markAsFullyLoaded();
	void updateModelColumns();
	ItemViewWidget* getViewWidget() const;
	QStandardItemModel* getModel() const;
	QString getAddonToolTip(Addon *addon);
	virtual QIcon getFallbackIcon() const;
	QIcon getAddonIcon(Addon *addon) const;
	virtual QVariant getAddonIdentifier(Addon *addon) const = 0;
	QModelIndexList getSelectedIndexes() const;
	virtual QVector<ModelColumn> getModelColumns() const;
	bool confirmAddonsRemoval(int amount) const;
	virtual bool canOpenAddons() const;
	virtual bool canReloadAddons() const;
	bool needsDetails() const;

protected slots:
	virtual void openAddons();
	virtual void reloadAddons();
	virtual void removeAddons() = 0;
	virtual void updateDetails();
	void showContextMenu(const QPoint &position);
	void setDetails(const QVector<DetailsEntry> &details);

private:
	QStandardItemModel *m_model;
	int m_loadingTimer;
	bool m_isLoading;
	bool m_needsDetails;
	Ui::AddonsPage *m_ui;

signals:
	void loadingStateChanged(WebWidget::LoadingState state);
	void needsActionsUpdate();
};

}

#endif
