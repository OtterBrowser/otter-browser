/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ConfigurationContentsWidget.h"
#include "./OverridesDialog.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkManagerFactory.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/ColorWidget.h"
#include "../../../ui/OptionWidget.h"

#include "ui_ConfigurationContentsWidget.h"

#include <QtCore/QMetaEnum>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ConfigurationOptionDelegate::ConfigurationOptionDelegate(bool shouldMarkAsModified, QObject *parent) : ItemDelegate(parent),
	m_shouldMarkAsModified(shouldMarkAsModified)
{
}

void ConfigurationOptionDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(index.data(ConfigurationContentsWidget::IdentifierRole).toInt()));

	option->text = SettingsManager::createDisplayValue(definition.identifier, index.data(Qt::DisplayRole));

	switch (definition.type)
	{
		case SettingsManager::ColorType:
			{
				QPixmap pixmap(option->fontMetrics.height(), option->fontMetrics.height());
				pixmap.fill(Qt::transparent);

				QPainter painter(&pixmap);
				painter.setRenderHints(QPainter::Antialiasing);

				ColorWidget::drawThumbnail(&painter, QColor(index.data(Qt::DisplayRole).toString()), option->palette, pixmap.rect());

				painter.end();

				option->features |= QStyleOptionViewItem::HasDecoration;
				option->decorationSize = pixmap.size();
				option->icon = QIcon(pixmap);
			}

			break;
		case SettingsManager::EnumerationType:
			{
				const QString value(index.data(Qt::DisplayRole).toString());

				if (definition.hasIcons())
				{
					option->features |= QStyleOptionViewItem::HasDecoration;
				}

				for (int i = 0; i < definition.choices.count(); ++i)
				{
					const SettingsManager::OptionDefinition::Choice choice(definition.choices.at(i));

					if (choice.value == value)
					{
						option->icon = choice.icon;

						break;
					}
				}
			}

			break;
		case SettingsManager::FontType:
			option->font = QFont(index.data(Qt::DisplayRole).toString());

			break;
		default:
			break;
	}
}

void ConfigurationOptionDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (widget && (!m_shouldMarkAsModified || !index.sibling(index.row(), 0).data(ConfigurationContentsWidget::IsModifiedRole).toBool()))
	{
		widget->setValue(index.data(Qt::EditRole));
	}
}

void ConfigurationOptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (!widget)
	{
		return;
	}

	const QModelIndex optionIndex(index.sibling(index.row(), 0));

	if (m_shouldMarkAsModified)
	{
		QFont font(optionIndex.data(Qt::FontRole).value<QFont>());
		font.setBold(widget->getValue() != widget->getDefaultValue());

		model->setData(optionIndex, font, Qt::FontRole);
	}

	model->setData(optionIndex, true, ConfigurationContentsWidget::IsModifiedRole);
	model->setData(index, widget->getValue(), Qt::EditRole);
}

QWidget* ConfigurationOptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(index.data(ConfigurationContentsWidget::IdentifierRole).toInt()));
	OptionWidget *widget(new OptionWidget(index.data(Qt::EditRole), definition.type, parent));
	widget->setDefaultValue(definition.defaultValue);

	if (definition.type == SettingsManager::EnumerationType)
	{
		widget->setChoices(definition.choices);
	}

	widget->setFocus();

	connect(widget, &OptionWidget::commitData, this, &ConfigurationOptionDelegate::commitData);

	return widget;
}

ConfigurationContentsWidget::ConfigurationContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::ConfigurationContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);

	NetworkManagerFactory::initialize();

	const QMetaEnum metaEnum(SettingsManager::getInstance()->metaObject()->enumerator(SettingsManager::getInstance()->metaObject()->indexOfEnumerator(QLatin1String("OptionType").data())));
	const EnumeratorMapper enumeratorMapper(metaEnum, QLatin1String("Type"));
	const QStringList options(SettingsManager::getOptions());
	QStandardItem *groupItem(nullptr);
	const QString fragment(parameters.value(QLatin1String("url")).toUrl().fragment());
	QModelIndex selectedIndex;
	bool canResetAll(false);

	for (int i = 0; i < options.count(); ++i)
	{
		const QString name(options.at(i));
		const int identifier(SettingsManager::getOptionIdentifier(name));
		const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(identifier));

		if (!definition.flags.testFlag(SettingsManager::OptionDefinition::IsEnabledFlag) || !definition.flags.testFlag(SettingsManager::OptionDefinition::IsVisibleFlag))
		{
			continue;
		}

		const QStringList nameParts(name.split(QLatin1Char('/')));

		if (!groupItem || groupItem->text() != nameParts.value(0))
		{
			groupItem = new QStandardItem(ThemesManager::createIcon(QLatin1String("inode-directory")), nameParts.value(0));

			m_model->appendRow(groupItem);
		}

		QList<QStandardItem*> optionItems({new QStandardItem(nameParts.last()), new QStandardItem(enumeratorMapper.mapToName(definition.type)), new QStandardItem(QString::number(SettingsManager::getOverridesCount(identifier))), new QStandardItem()});
		optionItems[0]->setFlags(optionItems[0]->flags() | Qt::ItemNeverHasChildren);
		optionItems[1]->setFlags(optionItems[1]->flags() | Qt::ItemNeverHasChildren);
		optionItems[2]->setFlags(optionItems[2]->flags() | Qt::ItemNeverHasChildren);
		optionItems[3]->setData(SettingsManager::getOption(identifier), Qt::EditRole);
		optionItems[3]->setData(QSize(-1, 30), Qt::SizeHintRole);
		optionItems[3]->setData(identifier, IdentifierRole);
		optionItems[3]->setData(name, NameRole);
		optionItems[3]->setFlags(optionItems[2]->flags() | Qt::ItemNeverHasChildren);

		if (definition.flags.testFlag(SettingsManager::OptionDefinition::RequiresRestartFlag))
		{
			optionItems[3]->setData(true, RequiresRestartRole);
		}

		if (!SettingsManager::isDefault(identifier))
		{
			QFont font(optionItems[0]->font());
			font.setBold(true);

			optionItems[0]->setFont(font);

			if (identifier != SettingsManager::Browser_MigrationsOption)
			{
				canResetAll = true;
			}
		}

		groupItem->appendRow(optionItems);

		if (!selectedIndex.isValid() && !fragment.isEmpty() && fragment == name)
		{
			selectedIndex = optionItems[0]->index();
		}
	}

	m_model->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Overrides"), tr("Value")});
	m_model->sort(0);

	m_ui->configurationViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->configurationViewWidget->setModel(m_model);
	m_ui->configurationViewWidget->setLayoutDirection(Qt::LeftToRight);
	m_ui->configurationViewWidget->setItemDelegateForColumn(3, new ConfigurationOptionDelegate(true, this));
	m_ui->configurationViewWidget->setFilterRoles({Qt::DisplayRole, NameRole});
	m_ui->configurationViewWidget->installEventFilter(this);
	m_ui->resetAllButton->setEnabled(canResetAll);

	if (isSidebarPanel())
	{
		m_ui->detailsWidget->hide();
	}

	if (selectedIndex.isValid())
	{
		m_ui->configurationViewWidget->expand(selectedIndex.parent());
		m_ui->configurationViewWidget->selectRow(selectedIndex);
	}

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &ConfigurationContentsWidget::handleOptionChanged);
	connect(SettingsManager::getInstance(), &SettingsManager::hostOptionChanged, this, &ConfigurationContentsWidget::handleHostOptionChanged);
	connect(m_ui->configurationViewWidget, &ItemViewWidget::customContextMenuRequested, this, &ConfigurationContentsWidget::showContextMenu);
	connect(m_ui->configurationViewWidget, &ItemViewWidget::needsActionsUpdate, this, &ConfigurationContentsWidget::updateActions);
	connect(m_ui->configurationViewWidget, &ItemViewWidget::isModifiedChanged, this, &ConfigurationContentsWidget::setModified);
	connect(m_ui->configurationViewWidget, &ItemViewWidget::clicked, this, [&](const QModelIndex &index)
	{
		if (index.parent().isValid() && index.column() != 3)
		{
			m_ui->configurationViewWidget->setCurrentIndex(index.sibling(index.row(), 3));
		}
	});
	connect(m_ui->configurationViewWidget, &ItemViewWidget::modified, this, [&]()
	{
		m_ui->resetAllButton->setEnabled(true);
		m_ui->saveAllButton->setEnabled(true);
	});
	connect(m_ui->configurationViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, [&](const QModelIndex &currentIndex, const QModelIndex &previousIndex)
	{
		if (previousIndex.parent().isValid() && previousIndex.column() == 3)
		{
			m_ui->configurationViewWidget->closePersistentEditor(previousIndex);
		}

		if (currentIndex.parent().isValid() && currentIndex.column() == 3)
		{
			m_ui->configurationViewWidget->openPersistentEditor(currentIndex);
		}
	});
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->configurationViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->resetAllButton, &QPushButton::clicked, this, [&]()
	{
		saveAll(true);
	});
	connect(m_ui->saveAllButton, &QPushButton::clicked, this, [&]()
	{
		saveAll(false);
	});
}

ConfigurationContentsWidget::~ConfigurationContentsWidget()
{
	delete m_ui;
}

void ConfigurationContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_model->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Overrides"), tr("Value")});
	}
}

void ConfigurationContentsWidget::print(QPrinter *printer)
{
	m_ui->configurationViewWidget->render(printer);
}

void ConfigurationContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->configurationViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

void ConfigurationContentsWidget::resetOption()
{
	const QModelIndex index(m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 3));

	if (index.isValid())
	{
		SettingsManager::setOption(index.data(IdentifierRole).toInt(), SettingsManager::getOptionDefinition(index.data(IdentifierRole).toInt()).defaultValue);

		m_model->setData(index.sibling(index.row(), 0), false, IsModifiedRole);

		updateActions();
	}
}

void ConfigurationContentsWidget::saveOption()
{
	const QModelIndex index(m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 3));

	if (index.isValid())
	{
		SettingsManager::setOption(index.data(IdentifierRole).toInt(), index.data(Qt::EditRole));

		m_model->setData(index.sibling(index.row(), 0), false, IsModifiedRole);

		updateActions();
	}
}

void ConfigurationContentsWidget::saveAll(bool reset)
{
	if (reset && QMessageBox::question(this, tr("Question"), tr("Do you really want to restore default values of all options?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		return;
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		const QModelIndex groupIndex(m_model->index(i, 0));
		const int optionAmount(m_model->rowCount(groupIndex));

		if (optionAmount == 0)
		{
			continue;
		}

		for (int j = 0; j < optionAmount; ++j)
		{
			const QModelIndex optionIndex(m_model->index(j, 0, groupIndex));
			const bool isModified(optionIndex.data(IsModifiedRole).toBool());

			if (!reset && !isModified)
			{
				continue;
			}

			const QModelIndex valueIndex(m_model->index(j, 3, groupIndex));
			const int identifier(valueIndex.data(IdentifierRole).toInt());
			const QVariant defaultValue(SettingsManager::getOptionDefinition(identifier).defaultValue);

			if (reset && identifier != SettingsManager::Browser_MigrationsOption && valueIndex.data(Qt::EditRole) != defaultValue)
			{
				SettingsManager::setOption(identifier, defaultValue);

				QFont font(optionIndex.data(Qt::FontRole).isNull() ? m_ui->configurationViewWidget->font() : optionIndex.data(Qt::FontRole).value<QFont>());
				font.setBold(false);

				m_model->setData(optionIndex, font, Qt::FontRole);
				m_model->setData(valueIndex, defaultValue, Qt::EditRole);
			}
			else if (!reset && isModified)
			{
				SettingsManager::setOption(identifier, valueIndex.data(Qt::EditRole));
			}

			if (isModified)
			{
				m_model->setData(optionIndex, false, IsModifiedRole);
			}
		}
	}

	m_ui->configurationViewWidget->setModified(false);
	m_ui->saveAllButton->setEnabled(false);

	if (reset)
	{
		m_ui->resetAllButton->setEnabled(false);
	}

	connect(m_ui->configurationViewWidget, &ItemViewWidget::needsActionsUpdate, this, &ConfigurationContentsWidget::updateActions);

	updateActions();
}

void ConfigurationContentsWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	const QModelIndex groupIndex(findGroup(identifier));
	const int optionAmount(m_model->rowCount(groupIndex));
	const bool wasModified(m_ui->configurationViewWidget->isModified());

	for (int i = 0; i < optionAmount; ++i)
	{
		const QModelIndex valueIndex(m_model->index(i, 3, groupIndex));

		if (valueIndex.data(IdentifierRole).toInt() != identifier)
		{
			continue;
		}

		const QModelIndex optionIndex(m_model->index(i, 0, groupIndex));

		if (!optionIndex.data(IsModifiedRole).toBool())
		{
			QFont font(optionIndex.data(Qt::FontRole).isNull() ? m_ui->configurationViewWidget->font() : optionIndex.data(Qt::FontRole).value<QFont>());
			font.setBold(!SettingsManager::isDefault(identifier));

			m_model->setData(optionIndex, font, Qt::FontRole);
			m_model->setData(valueIndex, value, Qt::EditRole);
		}

		break;
	}

	if (!wasModified)
	{
		m_ui->configurationViewWidget->setModified(false);
	}

	updateActions();
}

void ConfigurationContentsWidget::handleHostOptionChanged(int identifier)
{
	const QModelIndex groupIndex(findGroup(identifier));
	const int optionAmount(m_model->rowCount(groupIndex));
	const bool wasModified(m_ui->configurationViewWidget->isModified());

	for (int i = 0; i < optionAmount; ++i)
	{
		if (m_model->index(i, 3, groupIndex).data(IdentifierRole).toInt() == identifier)
		{
			m_model->setData(m_model->index(i, 2, groupIndex), QString::number(SettingsManager::getOverridesCount(identifier)), Qt::DisplayRole);

			break;
		}
	}

	if (!wasModified)
	{
		m_ui->configurationViewWidget->setModified(false);
	}
}

void ConfigurationContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->configurationViewWidget->indexAt(position));
	QMenu menu(this);

	if (index.isValid() && index.parent() != m_ui->configurationViewWidget->rootIndex())
	{
		const QModelIndex valueIndex(index.sibling(index.row(), 3));

		menu.addAction(tr("Copy Option Name"), this, [&]()
		{
			if (valueIndex.isValid())
			{
				QGuiApplication::clipboard()->setText(valueIndex.data(NameRole).toString());
			}
		});
		menu.addAction(tr("Copy Option Value"), this, [&]()
		{
			if (valueIndex.isValid())
			{
				QGuiApplication::clipboard()->setText(valueIndex.data(Qt::EditRole).toString());
			}
		});
		menu.addSeparator();
		menu.addAction(tr("Overrides…"), this, [&]()
		{
			if (valueIndex.isValid())
			{
				OverridesDialog(valueIndex.data(IdentifierRole).toInt(), this).exec();
			}
		});
		menu.addSeparator();
		menu.addAction(tr("Save Value"), this, &ConfigurationContentsWidget::saveOption)->setEnabled(index.sibling(index.row(), 0).data(IsModifiedRole).toBool());
		menu.addAction(tr("Restore Default Value"), this, &ConfigurationContentsWidget::resetOption)->setEnabled(!SettingsManager::isDefault(index.sibling(index.row(), 3).data(IdentifierRole).toInt()));
		menu.addSeparator();
	}

	menu.addAction(tr("Expand All"), m_ui->configurationViewWidget, &ItemViewWidget::expandAll);
	menu.addAction(tr("Collapse All"), m_ui->configurationViewWidget, &ItemViewWidget::collapseAll);
	menu.exec(m_ui->configurationViewWidget->mapToGlobal(position));
}

void ConfigurationContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->configurationViewWidget->selectionModel()->hasSelection() ? m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 3) : QModelIndex());
	const int identifier(index.data(IdentifierRole).toInt());

	if (identifier >= 0 && index.parent().isValid())
	{
		m_ui->nameLabelWidget->setText(SettingsManager::getOptionName(identifier));
		m_ui->currentValueLabelWidget->setText(SettingsManager::createDisplayValue(identifier, SettingsManager::getOption(identifier)));
		m_ui->defaultValueLabelWidget->setText(SettingsManager::createDisplayValue(identifier, SettingsManager::getOptionDefinition(identifier).defaultValue));
	}
	else
	{
		m_ui->nameLabelWidget->clear();
		m_ui->currentValueLabelWidget->clear();
		m_ui->defaultValueLabelWidget->clear();
	}
}

QString ConfigurationContentsWidget::getTitle() const
{
	return tr("Advanced Configuration");
}

QLatin1String ConfigurationContentsWidget::getType() const
{
	return QLatin1String("config");
}

QUrl ConfigurationContentsWidget::getUrl() const
{
	return {QLatin1String("about:config")};
}

QIcon ConfigurationContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("configuration"), false);
}

QModelIndex ConfigurationContentsWidget::findGroup(int identifier) const
{
	const QString group(SettingsManager::getOptionName(identifier).section(QLatin1Char('/'), 0, 0));

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		const QModelIndex groupIndex(m_model->index(i, 0));

		if (groupIndex.data(Qt::DisplayRole).toString() == group)
		{
			return groupIndex;
		}
	}

	return {};
}

bool ConfigurationContentsWidget::canClose()
{
	switch (QMessageBox::question(this, tr("Question"), tr("The settings have been changed.\nDo you want to save them?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel))
	{
		case QMessageBox::Cancel:
			return false;
		case QMessageBox::Yes:
			saveAll(false);

			break;
		default:
			break;
	}

	return true;
}

bool ConfigurationContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->configurationViewWidget && event->type() == QEvent::KeyPress)
	{
		const QModelIndex index(m_ui->configurationViewWidget->currentIndex());

		if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Right && index.parent().isValid())
		{
			m_ui->configurationViewWidget->setCurrentIndex(index.sibling(index.row(), 3));
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
