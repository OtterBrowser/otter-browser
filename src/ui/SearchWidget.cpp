#include "SearchWidget.h"
#include "PreferencesDialog.h"
#include "SearchDelegate.h"
#include "../core/SearchesManager.h"
#include "../core/SearchSuggester.h"
#include "../core/SessionsManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"

#include <QtWidgets/QLineEdit>

namespace Otter
{

SearchWidget::SearchWidget(QWidget *parent) : QComboBox(parent),
	m_model(new QStandardItemModel(this)),
	m_completer(new QCompleter(this)),
	m_suggester(NULL),
	m_index(0)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::PopupCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);

	setModel(m_model);
	setEditable(true);
	setInsertPolicy(QComboBox::NoInsert);
	setItemDelegate(new SearchDelegate(this));
	updateSearchEngines();
	optionChanged("Browser/SearchEnginesSuggestions", SettingsManager::getValue("Browser/SearchEnginesSuggestions"));
	lineEdit()->setCompleter(m_completer);

	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(updateSearchEngines()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));
	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	connect(lineEdit(), SIGNAL(returnPressed()), this, SLOT(sendRequest()));
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(sendRequest(QString)));
}

void SearchWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == "Browser/SearchEnginesSuggestions")
	{
		if (value.toBool() && !m_suggester)
		{
			m_suggester = new SearchSuggester(getCurrentSearchEngine(), this);

			connect(lineEdit(), SIGNAL(textEdited(QString)), m_suggester, SLOT(setQuery(QString)));
			connect(m_suggester, SIGNAL(suggestionsChanged(QList<SearchSuggestion>)), this, SLOT(updateSuggestions(QList<SearchSuggestion>)));
		}
		else if (!value.toBool() && m_suggester)
		{
			m_suggester->deleteLater();
			m_suggester = NULL;

			m_completer->model()->removeRows(0, m_completer->model()->rowCount());
		}
	}
}

void SearchWidget::currentSearchChanged(int index)
{
	if (itemData(index, (Qt::UserRole + 1)).toString().isEmpty())
	{
		setCurrentIndex(m_index);

		disconnect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));

		PreferencesDialog dialog("search", this);
		dialog.exec();

		connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));
	}
	else
	{
		m_index = index;

		lineEdit()->setPlaceholderText(tr("Search Using %1").arg(itemData(index, Qt::UserRole).toString()));
		lineEdit()->setText(m_query);

		if (m_suggester)
		{
			m_suggester->setEngine(getCurrentSearchEngine());
			m_suggester->setQuery(QString());
		}

		SessionsManager::markSessionModified();

		sendRequest();
	}
}

void SearchWidget::queryChanged(const QString &query)
{
	m_query = query;
}

void SearchWidget::sendRequest(const QString &query)
{
	if (!query.isEmpty())
	{
		m_query = query;
	}

	if (!m_query.isEmpty())
	{
		emit requestedSearch(m_query, itemData(currentIndex(), (Qt::UserRole + 1)).toString());
	}

	lineEdit()->clear();

	m_query = QString();
}

void SearchWidget::updateSearchEngines()
{
	disconnect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));

	m_model->clear();

	const QStringList engines = SearchesManager::getSearchEngines();

	for (int i = 0; i < engines.count(); ++i)
	{
		SearchInformation *search = SearchesManager::getSearchEngine(engines.at(i));

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

	setCurrentSearchEngine(SettingsManager::getValue("Browser/DefaultSearchEngine").toString());

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));
}

void SearchWidget::updateSuggestions(const QList<SearchSuggestion> &suggestions)
{
	if (suggestions.isEmpty())
	{
		if (m_completer->model())
		{
			m_completer->model()->removeRows(0, m_completer->model()->rowCount());
		}

		return;
	}

	QStandardItemModel *model = qobject_cast<QStandardItemModel*>(m_completer->model());

	if (!model)
	{
		model = new QStandardItemModel(m_completer);

		m_completer->setModel(model);
	}

	model->clear();

	for (int i = 0; i < suggestions.count(); ++i)
	{
		model->appendRow(new QStandardItem(suggestions.at(i).completion));
	}
}

void SearchWidget::setCurrentSearchEngine(const QString &engine)
{
	disconnect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));

	const QStringList engines = SearchesManager::getSearchEngines();

	if (engines.isEmpty())
	{
		return;
	}

	const int index = qMax(0, engines.indexOf(engine.isEmpty() ? SettingsManager::getValue("Browser/DefaultSearchEngine").toString() : engine));

	currentSearchChanged(index);
	setCurrentIndex(index);

	if (m_suggester)
	{
		m_suggester->setEngine(getCurrentSearchEngine());
	}

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSearchChanged(int)));
}

QString SearchWidget::getCurrentSearchEngine() const
{
	return itemData(currentIndex(), (Qt::UserRole + 1)).toString();
}

}
