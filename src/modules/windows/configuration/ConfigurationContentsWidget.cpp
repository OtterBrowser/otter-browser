/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ConfigurationContentsWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/OptionDelegate.h"

#include "ui_ConfigurationContentsWidget.h"

#include <QtCore/QSettings>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

ConfigurationContentsWidget::ConfigurationContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::ConfigurationContentsWidget)
{
	m_ui->setupUi(this);

	QSettings defaults(QLatin1String(":/schemas/options.ini"), QSettings::IniFormat, this);
	const QStringList groups = defaults.childGroups();

	for (int i = 0; i < groups.count(); ++i)
	{
		QStandardItem *groupItem = new QStandardItem(Utils::getIcon(QLatin1String("inode-directory")), groups.at(i));

		defaults.beginGroup(groups.at(i));

		const QStringList keys = defaults.childGroups();

		for (int j = 0; j < keys.count(); ++j)
		{
			const QString key = QStringLiteral("%1/%2").arg(groups.at(i)).arg(keys.at(j));
			const QString type = defaults.value(QStringLiteral("%1/type").arg(keys.at(j))).toString();
			const QVariant value = SettingsManager::getValue(key);
			QList<QStandardItem*> optionItems;
			optionItems.append(new QStandardItem(keys.at(j)));
			optionItems.append(new QStandardItem(type));
			optionItems.append(new QStandardItem(value.toString()));
			optionItems[0]->setFlags(optionItems[0]->flags() | Qt::ItemNeverHasChildren);
			optionItems[1]->setFlags(optionItems[1]->flags() | Qt::ItemNeverHasChildren);
			optionItems[2]->setData(QSize(-1, 30), Qt::SizeHintRole);
			optionItems[2]->setData(key, Qt::UserRole);
			optionItems[2]->setData(type, (Qt::UserRole + 1));
			optionItems[2]->setData(((type == QLatin1String("enumeration")) ? defaults.value(QStringLiteral("%1/choices").arg(keys.at(j))).toStringList() : QVariant()), (Qt::UserRole + 2));
			optionItems[2]->setFlags(optionItems[2]->flags() | Qt::ItemNeverHasChildren);

			if (value != SettingsManager::getDefaultValue(key))
			{
				QFont font = optionItems[0]->font();
				font.setBold(true);

				optionItems[0]->setFont(font);
			}

			groupItem->appendRow(optionItems);
		}

		defaults.endGroup();

		m_model->appendRow(groupItem);
	}

	QStringList labels;
	labels << tr("Name") << tr("Type") << tr("Value");

	m_model->setHorizontalHeaderLabels(labels);
	m_model->sort(0);

	QSet<int> filterRoles;
	filterRoles << Qt::DisplayRole << Qt::UserRole;

	m_ui->configurationViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->configurationViewWidget->setModel(m_model);
	m_ui->configurationViewWidget->setItemDelegateForColumn(2, new OptionDelegate(false, this));
	m_ui->configurationViewWidget->setFilterRoles(filterRoles);
	m_ui->configurationViewWidget->header()->setTextElideMode(Qt::ElideRight);
	m_ui->filterLineEdit->installEventFilter(this);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(m_ui->configurationViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
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

void ConfigurationContentsWidget::optionChanged(const QString &option, const QVariant &value)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (!groupItem || !QString(option).startsWith(groupItem->text()))
		{
			continue;
		}

		for (int j = 0; j < groupItem->rowCount(); ++j)
		{
			QStandardItem *optionItem = groupItem->child(j, 0);

			if (optionItem && option == QStringLiteral("%1/%2").arg(groupItem->text()).arg(optionItem->text()))
			{
				QFont font = optionItem->font();
				font.setBold(value != SettingsManager::getDefaultValue(option));

				optionItem->setFont(font);

				groupItem->child(j, 2)->setText(value.toString());

				break;
			}
		}
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
	const QModelIndex index = m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2);

	if (index.isValid())
	{
		QApplication::clipboard()->setText(index.data(Qt::UserRole).toString());
	}
}

void ConfigurationContentsWidget::copyOptionValue()
{
	const QModelIndex index = m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2);

	if (index.isValid())
	{
		QApplication::clipboard()->setText(index.data(Qt::EditRole).toString());
	}
}

void ConfigurationContentsWidget::restoreDefaults()
{
	const QModelIndex index = m_ui->configurationViewWidget->currentIndex().sibling(m_ui->configurationViewWidget->currentIndex().row(), 2);

	if (index.isValid())
	{
		SettingsManager::setValue(index.data(Qt::UserRole).toString(), SettingsManager::getDefaultValue(index.data(Qt::UserRole).toString()));

		m_ui->configurationViewWidget->setCurrentIndex(QModelIndex());
		m_ui->configurationViewWidget->setCurrentIndex(index);
	}
}

void ConfigurationContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index = m_ui->configurationViewWidget->indexAt(point);

	if (index.isValid() && index.parent() != m_ui->configurationViewWidget->rootIndex())
	{
		QMenu menu(this);
		menu.addAction(tr("Copy Option Name"), this, SLOT(copyOptionName()));
		menu.addAction(tr("Copy Option Value"), this, SLOT(copyOptionValue()));
		menu.addSeparator();
		menu.addAction(tr("Restore Default Value"), this, SLOT(restoreDefaults()))->setEnabled(index.sibling(index.row(), 2).data(Qt::EditRole) != SettingsManager::getDefaultValue(index.sibling(index.row(), 2).data(Qt::UserRole).toString()));
		menu.exec(m_ui->configurationViewWidget->mapToGlobal(point));
	}
}

QString ConfigurationContentsWidget::getTitle() const
{
	return tr("Configuration Manager");
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
	return Utils::getIcon(QLatin1String("configuration"), false);
}

bool ConfigurationContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->filterLineEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Escape)
		{
			m_ui->filterLineEdit->clear();
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
