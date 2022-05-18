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

#include "AddonsPage.h"
#include "../../../core/ThemesManager.h"

#include "ui_AddonsPage.h"

namespace Otter
{

AddonsPage::AddonsPage(QWidget *parent) : CategoryPage(parent),
	m_loadingTimer(0),
	m_isLoading(true),
	m_ui(new Ui::AddonsPage)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->addonsViewWidget->setModel(new QStandardItemModel(this));
	m_ui->addonsViewWidget->setViewMode(ItemViewWidget::ListView);
	m_ui->addonsViewWidget->installEventFilter(this);

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->addonsViewWidget, &ItemViewWidget::setFilterString);
}

AddonsPage::~AddonsPage()
{
	delete m_ui;
}

void AddonsPage::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_loadingTimer)
	{
		delayedLoad();

		killTimer(m_loadingTimer);

		m_loadingTimer = 0;

		markAsFullyLoaded();
	}
	else
	{
		CategoryPage::timerEvent(event);
	}
}

void AddonsPage::changeEvent(QEvent *event)
{
	CategoryPage::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void AddonsPage::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	Q_UNUSED(trigger)

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->addonsViewWidget->selectAll();

			break;
		case ActionsManager::DeleteAction:
			removeAddons();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->addonsViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void AddonsPage::addAddon(Addon *addon, const QMap<int, QVariant> &metaData)
{
	QStandardItem *item(new QStandardItem((addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), (addon->getVersion().isEmpty() ? addon->getTitle() : QStringLiteral("%1 %2").arg(addon->getTitle(), addon->getVersion()))));
	item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
	item->setCheckable(true);
	item->setCheckState(addon->isEnabled() ? Qt::Checked : Qt::Unchecked);
	item->setToolTip(addon->getDescription());

	m_ui->addonsViewWidget->getSourceModel()->appendRow(item);
	m_ui->addonsViewWidget->getSourceModel()->setItemData(item->index(), metaData);
}

void AddonsPage::load()
{
	if (!wasLoaded())
	{
		emit loadingStateChanged(WebWidget::OngoingLoadingState);

		m_loadingTimer = startTimer(200);

		markAsLoaded();
	}
}

void AddonsPage::markAsFullyLoaded()
{
	m_isLoading = false;

	emit loadingStateChanged(WebWidget::FinishedLoadingState);
}

QStandardItemModel *AddonsPage::getModel() const
{
	return m_ui->addonsViewWidget->getSourceModel();
}

QIcon AddonsPage::getFallbackIcon() const
{
	return ThemesManager::createIcon(QLatin1String("unknown"), false);
}

ActionsManager::ActionDefinition::State AddonsPage::getActionState(int identifier, const QVariantMap &parameters) const
{
	Q_UNUSED(parameters)

	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			state.isEnabled = true;

			break;
		case ActionsManager::DeleteAction:
			state.isEnabled = (m_ui->addonsViewWidget->selectionModel() && !m_ui->addonsViewWidget->selectionModel()->selectedIndexes().isEmpty());

			break;
		default:
			break;
	}

	return state;
}

}
