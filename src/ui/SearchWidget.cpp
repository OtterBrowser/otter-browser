#include "SearchWidget.h"
#include "SearchDelegate.h"
#include "../core/SearchesManager.h"
#include "../core/Utils.h"

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QLineEdit>

namespace Otter
{

SearchWidget::SearchWidget(QWidget *parent) : QComboBox(parent)
{
	QStandardItemModel *model = new QStandardItemModel(this);
	const QStringList searches = SearchesManager::getSearches();

	for (int i = 0; i < searches.count(); ++i)
	{
		SearchInformation *search = SearchesManager::getSearch(searches.at(i));

		if (search)
		{
			QStandardItem *item = new QStandardItem((search->icon.isNull() ? Utils::getIcon("edit-find") : search->icon), QString());
			item->setData(search->title, Qt::UserRole);
			item->setData(search->identifier, (Qt::UserRole + 1));
			item->setData(QSize(-1, 22), Qt::SizeHintRole);

			model->appendRow(item);
		}
	}

	setEditable(true);
	setModel(model);
	setInsertPolicy(QComboBox::NoInsert);
	setItemDelegate(new SearchDelegate(this));
	setCurrentIndex(0);

	m_query = QString();

	currentSearchChanged(0);

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));
	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	connect(lineEdit(), SIGNAL(returnPressed()), this, SLOT(sendRequest()));
}

void SearchWidget::currentSearchChanged(int index)
{
	lineEdit()->setPlaceholderText(tr("Search Using %1").arg(itemData(index, Qt::UserRole).toString()));
	lineEdit()->setText(m_query);

	sendRequest();
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
