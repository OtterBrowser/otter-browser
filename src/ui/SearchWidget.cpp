#include "SearchWidget.h"
#include "PreferencesDialog.h"
#include "SearchDelegate.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include <QtWidgets/QLineEdit>

namespace Otter
{

SearchWidget::SearchWidget(QWidget *parent) : QComboBox(parent),
	m_model(new QStandardItemModel(this)),
	m_index(0)
{
	setEditable(true);
	setInsertPolicy(QComboBox::NoInsert);
	setItemDelegate(new SearchDelegate(this));
	optionChanged("Browser/SearchEnginesOrder");

	m_query = QString();

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));
	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	connect(lineEdit(), SIGNAL(returnPressed()), this, SLOT(sendRequest()));
}

void SearchWidget::optionChanged(const QString &option)
{
	if (option != "Browser/SearchEnginesOrder")
	{
		return;
	}

	m_model->clear();

	const QStringList engines = SearchesManager::getEngines();

	for (int i = 0; i < engines.count(); ++i)
	{
		SearchInformation *search = SearchesManager::getEngine(engines.at(i));

		if (search)
		{
			QStandardItem *item = new QStandardItem((search->icon.isNull() ? Utils::getIcon("edit-find") : search->icon), QString());
			item->setData(search->title, Qt::UserRole);
			item->setData(search->identifier, (Qt::UserRole + 1));
			item->setData(search->shortcut, (Qt::UserRole + 2));
			item->setData(QSize(-1, 22), Qt::SizeHintRole);

			m_model->appendRow(item);
		}
	}

	if (engines.count() > 0)
	{
		setEnabled(true);

		QStandardItem *separatorItem = new QStandardItem(Utils::getIcon("configure"), QString());
		separatorItem->setData("separator", Qt::AccessibleDescriptionRole);
		separatorItem->setData(QSize(-1, 10), Qt::SizeHintRole);

		QStandardItem *manageItem = new QStandardItem(Utils::getIcon("configure"), QString());
		manageItem->setData(tr("Manage Search Engines..."), Qt::UserRole);
		manageItem->setData(QSize(-1, 22), Qt::SizeHintRole);

		m_model->appendRow(separatorItem);
		m_model->appendRow(manageItem);
	}
	else
	{
		setEnabled(false);

		lineEdit()->setPlaceholderText(QString());
	}

	const int index = qMax(0, engines.indexOf(SettingsManager::getValue("Browser/DefaultSearchEngine").toString()));

	setModel(m_model);
	currentSearchChanged(index);
	setCurrentIndex(index);
}

void SearchWidget::currentSearchChanged(int index)
{
	if (itemData(index, (Qt::UserRole + 1)).toString().isEmpty())
	{
		setCurrentIndex(m_index);

		PreferencesDialog dialog("search", this);
		dialog.exec();
	}
	else
	{
		m_index = index;

		lineEdit()->setPlaceholderText(tr("Search Using %1").arg(itemData(index, Qt::UserRole).toString()));
		lineEdit()->setText(m_query);

		sendRequest();
	}
}

void SearchWidget::queryChanged(const QString &query)
{
	m_query = query;
}

void SearchWidget::sendRequest()
{
	if (!m_query.isEmpty())
	{
		emit requestedSearch(m_query, itemData(currentIndex(), (Qt::UserRole + 1)).toString());
	}

	lineEdit()->clear();

	m_query = QString();
}

}
