#include "ConfigurationContentsWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/OptionDelegate.h"

#include "ui_ConfigurationContentsWidget.h"

#include <QtCore/QSettings>

namespace Otter
{

ConfigurationContentsWidget::ConfigurationContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::ConfigurationContentsWidget)
{
	m_ui->setupUi(this);

	m_model->sort(0);

	QSettings defaults(":/files/options.ini", QSettings::IniFormat, this);
	const QStringList groups = defaults.childGroups();

	for (int i = 0; i < groups.count(); ++i)
	{
		QStandardItem *groupItem = new QStandardItem(Utils::getIcon("inode-directory"), groups.at(i));

		defaults.beginGroup(groups.at(i));

		const QStringList keys = defaults.childGroups();

		for (int j = 0; j < keys.count(); ++j)
		{
			const QVariant defaultValue = defaults.value(QString("%1/value").arg(keys.at(j)));
			const QVariant value = SettingsManager::getValue(QString("%1/%2").arg(groups.at(i)).arg(keys.at(j)), defaultValue);
			QList<QStandardItem*> optionItems;
			optionItems.append(new QStandardItem(keys.at(j)));
			optionItems.append(new QStandardItem(defaults.value(QString("%1/type").arg(keys.at(j))).toString()));
			optionItems.append(new QStandardItem(value.toString()));
			optionItems[2]->setData(QSize(-1, 30), Qt::SizeHintRole);

			if (value != defaultValue)
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

	m_ui->configurationView->setModel(m_model);
	m_ui->configurationView->setItemDelegateForColumn(2, new OptionDelegate(this));
	m_ui->configurationView->header()->setTextElideMode(Qt::ElideRight);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(m_ui->configurationView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(currentChanged(QModelIndex,QModelIndex)));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterConfiguration(QString)));
}

ConfigurationContentsWidget::~ConfigurationContentsWidget()
{
	delete m_ui;
}

void ConfigurationContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void ConfigurationContentsWidget::print(QPrinter *printer)
{
	m_ui->configurationView->render(printer);
}

void ConfigurationContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void ConfigurationContentsWidget::setHistory(const HistoryInformation &history)
{
	Q_UNUSED(history)
}

void ConfigurationContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void ConfigurationContentsWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
}

void ConfigurationContentsWidget::filterConfiguration(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (!groupItem)
		{
			continue;
		}

		bool found = filter.isEmpty();

		for (int j = 0; j < groupItem->rowCount(); ++j)
		{
			QStandardItem *optionItem = groupItem->child(j, 0);

			if (!optionItem)
			{
				continue;
			}

			const bool match = (filter.isEmpty() || QString("%1/%2").arg(groupItem->text()).arg(optionItem->text()).contains(filter, Qt::CaseInsensitive) || groupItem->child(j, 2)->text().contains(filter, Qt::CaseInsensitive));

			if (match)
			{
				found = true;
			}

			m_ui->configurationView->setRowHidden(j, groupItem->index(), !match);
		}

		m_ui->configurationView->setRowHidden(i, m_model->invisibleRootItem()->index(), !found);
		m_ui->configurationView->setExpanded(groupItem->index(), !filter.isEmpty());
	}
}

void ConfigurationContentsWidget::currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
	m_ui->configurationView->closePersistentEditor(previousIndex.parent().child(previousIndex.row(), 2));

	if (currentIndex.parent().isValid())
	{
		m_ui->configurationView->openPersistentEditor(currentIndex.parent().child(currentIndex.row(), 2));
	}
}

void ConfigurationContentsWidget::optionChanged(const QString &option, const QVariant &value)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *groupItem = m_model->item(i, 0);

		if (!groupItem || !option.startsWith(groupItem->text()))
		{
			continue;
		}

		for (int j = 0; j < groupItem->rowCount(); ++j)
		{
			QStandardItem *optionItem = groupItem->child(j, 0);

			if (optionItem && option == QString("%1/%2").arg(groupItem->text()).arg(optionItem->text()))
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

ContentsWidget* ConfigurationContentsWidget::clone(Window *window)
{
	Q_UNUSED(window)

	return NULL;
}

QAction* ConfigurationContentsWidget::getAction(WindowAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* ConfigurationContentsWidget::getUndoStack()
{
	return NULL;
}

QString ConfigurationContentsWidget::getTitle() const
{
	return tr("Configuration Manager");
}

QString ConfigurationContentsWidget::getType() const
{
	return "config";
}

QUrl ConfigurationContentsWidget::getUrl() const
{
	return QUrl("about:config");
}

QIcon ConfigurationContentsWidget::getIcon() const
{
	return QIcon(":/icons/configuration.png");
}

QPixmap ConfigurationContentsWidget::getThumbnail() const
{
	return QPixmap();
}

HistoryInformation ConfigurationContentsWidget::getHistory() const
{
	HistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	HistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
}

int ConfigurationContentsWidget::getZoom() const
{
	return 100;
}

bool ConfigurationContentsWidget::canZoom() const
{
	return false;
}

bool ConfigurationContentsWidget::isClonable() const
{
	return false;
}

bool ConfigurationContentsWidget::isLoading() const
{
	return false;
}

bool ConfigurationContentsWidget::isPrivate() const
{
	return false;
}

}
