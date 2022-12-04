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
#include "../../../ui/TextLabelWidget.h"

#include "ui_AddonsPage.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>

namespace Otter
{

AddonsPage::AddonsPage(bool needsDetails, QWidget *parent) : CategoryPage(parent),
	m_loadingTimer(0),
	m_isLoading(true),
	m_needsDetails(needsDetails),
	m_ui(new Ui::AddonsPage)
{
	QStandardItemModel *model(new QStandardItemModel(this));
	model->setHorizontalHeaderLabels({tr("Title"), tr("Version")});
	model->setHeaderData(0, Qt::Horizontal, 250, HeaderViewWidget::WidthRole);

	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->addonsViewWidget->setModel(model);
	m_ui->addonsViewWidget->setViewMode(ItemViewWidget::ListView);
	m_ui->addonsViewWidget->installEventFilter(this);
	m_ui->detailsWidget->setVisible(m_needsDetails);

	connect(this, &AddonsPage::settingsModified, this, [&]()
	{
		m_ui->saveButton->setEnabled(true);
	});
	connect(m_ui->addButton, &QPushButton::clicked, this, &AddonsPage::addAddon);
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->addonsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->addonsViewWidget, &ItemViewWidget::customContextMenuRequested, this, &AddonsPage::showContextMenu);
	connect(m_ui->addonsViewWidget, &ItemViewWidget::needsActionsUpdate, this, [&]()
	{
		if (m_needsDetails)
		{
			updateDetails();
		}

		emit needsActionsUpdate();
	});
	connect(m_ui->saveButton, &QPushButton::clicked, this, [&]()
	{
		save();

		m_ui->saveButton->setEnabled(false);
	});
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
		m_ui->addonsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Title"), tr("Version")});

		if (m_needsDetails)
		{
			updateDetails();
		}
	}
}

void AddonsPage::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	Q_UNUSED(parameters)
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

void AddonsPage::print(QPrinter *printer)
{
	m_ui->addonsViewWidget->render(printer);
}

void AddonsPage::addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData)
{
	QList<QStandardItem*> items({new QStandardItem((addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), addon->getTitle()), new QStandardItem(addon->getVersion())});
	items[0]->setData(getAddonIdentifier(addon), IdentifierRole);
	items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
	items[0]->setCheckable(true);
	items[0]->setCheckState(addon->isEnabled() ? Qt::Checked : Qt::Unchecked);
	items[0]->setToolTip(addon->getDescription());
	items[1]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
	items[1]->setToolTip(addon->getDescription());

	m_ui->addonsViewWidget->getSourceModel()->appendRow(items);
	m_ui->addonsViewWidget->getSourceModel()->setItemData(items[0]->index(), metaData);
}

void AddonsPage::updateAddonEntry(Addon *addon)
{
	const QVariant identifier(getAddonIdentifier(addon));

	for (int i = 0; i < m_ui->addonsViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->addonsViewWidget->getIndex(i));

		if (index.data(IdentifierRole) == identifier)
		{
			m_ui->addonsViewWidget->setData(index, (addon->getIcon().isNull() ? getFallbackIcon() : addon->getIcon()), Qt::DecorationRole);
			m_ui->addonsViewWidget->setData(index, addon->getTitle(), Qt::DisplayRole);
			m_ui->addonsViewWidget->setData(index, addon->getDescription(), Qt::ToolTipRole);
			m_ui->addonsViewWidget->setData(index.sibling(i, 1), addon->getVersion(), Qt::DisplayRole);
			m_ui->addonsViewWidget->setData(index.sibling(i, 1), addon->getDescription(), Qt::ToolTipRole);

			return;
		}
	}
}

void AddonsPage::openAddons()
{
}

void AddonsPage::reloadAddons()
{
}

void AddonsPage::updateDetails()
{
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

void AddonsPage::showContextMenu(const QPoint &position)
{
	QMenu menu(this);
	menu.addAction(tr("Add Addon…"), this, &AddonsPage::addAddon);

	if (m_ui->addonsViewWidget->selectionModel()->hasSelection())
	{
		menu.addSeparator();
		menu.addAction(tr("Open Addon File"), this, &AddonsPage::openAddons)->setEnabled(canOpenAddons());
		menu.addAction(tr("Reload Addon"), this, &AddonsPage::reloadAddons)->setEnabled(canReloadAddons());
		menu.addSeparator();
		menu.addAction(tr("Remove Addon…"), this, &AddonsPage::removeAddons);
	}

	menu.exec(m_ui->addonsViewWidget->mapToGlobal(position));
}

void AddonsPage::setDetails(const QVector<AddonsPage::DetailsEntry> &details)
{
	if (m_ui->formLayout->rowCount() > 0)
	{
		for (int i = 0; i < details.count(); ++i)
		{
			const DetailsEntry entry(details.at(i));
			QLayoutItem *labelItem(m_ui->formLayout->itemAt(i, QFormLayout::LabelRole));
			QLayoutItem *fieldItem(m_ui->formLayout->itemAt(i, QFormLayout::FieldRole));

			if (!labelItem || !fieldItem)
			{
				continue;
			}

			QLabel *label(qobject_cast<QLabel*>(labelItem->widget()));
			TextLabelWidget *textWidget(qobject_cast<TextLabelWidget*>(fieldItem->widget()));

			if (!label || !textWidget)
			{
				continue;
			}

			label->setText(entry.label);

			textWidget->setText(entry.value);

			if (entry.isUrl)
			{
				textWidget->setUrl({entry.value});
			}
		}
	}
	else
	{
		for (int i = 0; i < details.count(); ++i)
		{
			const DetailsEntry entry(details.at(i));
			TextLabelWidget *textWidget(new TextLabelWidget(m_ui->detailsWidget));
			textWidget->setText(entry.value);

			if (entry.isUrl)
			{
				textWidget->setUrl({entry.value});
			}

			m_ui->formLayout->addRow(entry.label, textWidget);
		}
	}
}

ItemViewWidget* AddonsPage::getViewWidget() const
{
	return m_ui->addonsViewWidget;
}

QStandardItemModel* AddonsPage::getModel() const
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

QModelIndexList AddonsPage::getSelectedIndexes() const
{
	return m_ui->addonsViewWidget->selectionModel()->selectedIndexes();
}

WebWidget::LoadingState AddonsPage::getLoadingState() const
{
	return (m_isLoading ? WebWidget::OngoingLoadingState : WebWidget::FinishedLoadingState);
}

bool AddonsPage::canOpenAddons() const
{
	return false;
}

bool AddonsPage::canReloadAddons() const
{
	return false;
}

bool AddonsPage::needsDetails() const
{
	return m_needsDetails;
}

}
