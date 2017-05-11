/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/ActionsManager.h"
#include "../../../core/NetworkManagerFactory.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/OptionWidget.h"

#include "ui_ConfigurationContentsWidget.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>

namespace Otter
{

ConfigurationOptionDelegate::ConfigurationOptionDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ConfigurationOptionDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(index.data(ConfigurationContentsWidget::IdentifierRole).toString())));

	option->text = SettingsManager::createDisplayValue(definition.identifier, index.data(Qt::DisplayRole));

	switch (definition.type)
	{
		case SettingsManager::ColorType:
			{
				const QColor color(index.data(Qt::DisplayRole).toString());
				QPixmap icon(option->fontMetrics.height(), option->fontMetrics.height());
				icon.fill(Qt::transparent);

				QPainter iconPainter(&icon);
				iconPainter.setRenderHints(QPainter::Antialiasing);

				if (color.alpha() < 255)
				{
					QPixmap pixmap(10, 10);
					pixmap.fill(Qt::white);

					QPainter pixmapPainter(&pixmap);
					pixmapPainter.setBrush(Qt::gray);
					pixmapPainter.setPen(Qt::NoPen);
					pixmapPainter.drawRect(0, 0, 5, 5);
					pixmapPainter.drawRect(5, 5, 5, 5);
					pixmapPainter.end();

					iconPainter.setBrush(pixmap);
					iconPainter.setPen(Qt::NoPen);
					iconPainter.drawRoundedRect(icon.rect(), 2, 2);
				}

				iconPainter.setBrush(color);
				iconPainter.setPen(option->palette.color(QPalette::Button));
				iconPainter.drawRoundedRect(icon.rect(), 2, 2);
				iconPainter.end();

				option->features |= QStyleOptionViewItem::HasDecoration;
				option->decorationSize = icon.size();
				option->icon = QIcon(icon);
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
					if (definition.choices.at(i).value == value)
					{
						option->icon = definition.choices.at(i).icon;

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

void ConfigurationOptionDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void ConfigurationOptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (widget)
	{
		const QModelIndex optionIndex(index.sibling(index.row(), 0));
		QFont font(optionIndex.data(Qt::FontRole).value<QFont>());
		font.setBold(widget->getValue() != widget->getDefaultValue());

		model->setData(index, widget->getValue());
		model->setData(optionIndex, font, Qt::FontRole);
		model->setData(optionIndex, true, ConfigurationContentsWidget::IsModifiedRole);
	}
}

QWidget* ConfigurationOptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const QString name(index.data(ConfigurationContentsWidget::IdentifierRole).toString());
	const int identifier(SettingsManager::getOptionIdentifier(name));
	const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(identifier));
	OptionWidget *widget(new OptionWidget(name, index.data(Qt::EditRole), definition.type, parent));
	widget->setIndex(index);
	widget->setDefaultValue(definition.defaultValue);

	if (definition.type == SettingsManager::EnumerationType)
	{
		widget->setChoices(definition.choices);
	}

	connect(widget, SIGNAL(commitData(QWidget*)), this, SIGNAL(commitData(QWidget*)));

	return widget;
}

ConfigurationContentsWidget::ConfigurationContentsWidget(const QVariantMap &parameters, Window *window) : ContentsWidget(parameters, window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::ConfigurationContentsWidget)
{
	m_ui->setupUi(this);

	NetworkManagerFactory::initialize();

	const QMetaEnum metaEnum(SettingsManager::getInstance()->metaObject()->enumerator(SettingsManager::getInstance()->metaObject()->indexOfEnumerator(QLatin1String("OptionType").data())));
	const QStringList options(SettingsManager::getOptions());
	QStandardItem *groupItem(nullptr);

	for (int i = 0; i < options.count(); ++i)
	{
		const int identifier(SettingsManager::getOptionIdentifier(options.at(i)));
		const QStringList option(options.at(i).split(QLatin1Char('/')));
		const QVariant value(SettingsManager::getOption(identifier));
		const SettingsManager::OptionDefinition definition(SettingsManager::getOptionDefinition(identifier));

		if (!definition.flags.testFlag(SettingsManager::IsEnabledFlag) || !definition.flags.testFlag(SettingsManager::IsVisibleFlag))
		{
			continue;
		}

		if (!groupItem || groupItem->text() != option.first())
		{
			groupItem = new QStandardItem(ThemesManager::createIcon(QLatin1String("inode-directory")), option.first());

			m_model->appendRow(groupItem);
		}

		QString type(metaEnum.valueToKey(definition.type));
		type.chop(4);

		QList<QStandardItem*> optionItems({new QStandardItem(option.last()), new QStandardItem(type.toLower()), new QStandardItem((value.type() == QVariant::StringList) ? value.toStringList().join(QLatin1String(", ")) : value.toString())});
		optionItems[0]->setFlags(optionItems[0]->flags() | Qt::ItemNeverHasChildren);
		optionItems[1]->setFlags(optionItems[1]->flags() | Qt::ItemNeverHasChildren);
		optionItems[2]->setData(QSize(-1, 30), Qt::SizeHintRole);
		optionItems[2]->setData(options.at(i), IdentifierRole);
		optionItems[2]->setFlags(optionItems[2]->flags() | Qt::ItemNeverHasChildren);

		if (value != definition.defaultValue)
		{
			QFont font(optionItems[0]->font());
			font.setBold(true);

			optionItems[0]->setFont(font);
		}

		groupItem->appendRow(optionItems);
	}

	m_model->setHorizontalHeaderLabels(QStringList({tr("Name"), tr("Type"), tr("Value")}));
	m_model->sort(0);

	m_ui->configurationViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->configurationViewWidget->setModel(m_model);
	m_ui->configurationViewWidget->setLayoutDirection(Qt::LeftToRight);
	m_ui->configurationViewWidget->setItemDelegateForColumn(2, new ConfigurationOptionDelegate(this));
	m_ui->configurationViewWidget->setFilterRoles(QSet<int>({Qt::DisplayRole, IdentifierRole}));
	m_ui->filterLineEdit->installEventFilter(this);

	if (isSidebarPanel())
	{
		m_ui->detailsWidget->hide();
	}

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
	connect(m_ui->configurationViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->configurationViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActions()));
	connect(m_ui->configurationViewWidget->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(currentChanged(QModelIndex,QModelIndex)));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->configurationViewWidget, SLOT(setFilterString(QString)));
}

ConfigurationContentsWidget::~ConfigurationContentsWidget()
{
	delete m_ui;
}

void ConfigurationContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ConfigurationContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->configurationViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void ConfigurationContentsWidget::print(QPrinter *printer)
{
	m_ui->configurationViewWidget->render(printer);
}

void ConfigurationContentsWidget::currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
	m_ui->configurationViewWidget->closePersistentEditor(previousIndex.parent().child(previousIndex.row(), 2));

	if (currentIndex.parent().isValid())
	{
		m_ui->configurationViewWidget->openPersistentEditor(currentIndex.parent().child(currentIndex.row(), 2));
	}
}

void ConfigurationContentsWidget::copyOptionName()
{
	const QModelIndex index(m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2));

	if (index.isValid())
	{
		QApplication::clipboard()->setText(index.data(IdentifierRole).toString());
	}
}

void ConfigurationContentsWidget::copyOptionValue()
{
	const QModelIndex index(m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2));

	if (index.isValid())
	{
		QApplication::clipboard()->setText(index.data(Qt::EditRole).toString());
	}
}

void ConfigurationContentsWidget::restoreDefaults()
{
	const QModelIndex index(m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2));

	if (index.isValid())
	{
		const int identifier(SettingsManager::getOptionIdentifier(index.data(IdentifierRole).toString()));

		SettingsManager::setValue(identifier, SettingsManager::getOptionDefinition(identifier).defaultValue);

		m_ui->configurationViewWidget->setCurrentIndex(QModelIndex());
		m_ui->configurationViewWidget->setCurrentIndex(index);
	}
}

void ConfigurationContentsWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	const QString name(SettingsManager::getOptionName(identifier));

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem(m_model->item(i, 0));

		if (!groupItem || !name.startsWith(groupItem->text()))
		{
			continue;
		}

		for (int j = 0; j < groupItem->rowCount(); ++j)
		{
			QStandardItem *optionItem(groupItem->child(j, 0));

			if (optionItem && name == QStringLiteral("%1/%2").arg(groupItem->text()).arg(optionItem->text()))
			{
				QFont font(optionItem->font());
				font.setBold(value != SettingsManager::getOptionDefinition(identifier).defaultValue);

				optionItem->setFont(font);

				groupItem->child(j, 2)->setText(value.toString());

				break;
			}
		}
	}

	updateActions();
}

void ConfigurationContentsWidget::showContextMenu(const QPoint &position)
{
	const QModelIndex index(m_ui->configurationViewWidget->indexAt(position));
	QMenu menu(this);

	if (index.isValid() && index.parent() != m_ui->configurationViewWidget->rootIndex())
	{
		menu.addAction(tr("Copy Option Name"), this, SLOT(copyOptionName()));
		menu.addAction(tr("Copy Option Value"), this, SLOT(copyOptionValue()));
		menu.addSeparator();
		menu.addAction(tr("Restore Default Value"), this, SLOT(restoreDefaults()))->setEnabled(index.sibling(index.row(), 2).data(Qt::EditRole) != SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(index.sibling(index.row(), 2).data(IdentifierRole).toString())).defaultValue);
		menu.addSeparator();
	}

	menu.addAction(tr("Expand All"), m_ui->configurationViewWidget, SLOT(expandAll()));
	menu.addAction(tr("Collapse All"), m_ui->configurationViewWidget, SLOT(collapseAll()));
	menu.exec(m_ui->configurationViewWidget->mapToGlobal(position));
}

void ConfigurationContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->configurationViewWidget->selectionModel()->hasSelection() ? m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2) : QModelIndex());
	const int identifier(SettingsManager::getOptionIdentifier(index.data(IdentifierRole).toString()));

	if (identifier >= 0)
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
	return QUrl(QLatin1String("about:config"));
}

QIcon ConfigurationContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("configuration"), false);
}

bool ConfigurationContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->filterLineEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent->key() == Qt::Key_Escape)
		{
			m_ui->filterLineEdit->clear();
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
