#include "SearchWidget.h"
#include "PreferencesDialog.h"
#include "SearchDelegate.h"
#include "../core/SearchesManager.h"
#include "../core/SearchSuggester.h"
#include "../core/SessionsManager.h"
#include "../core/SettingsManager.h"

#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QLineEdit>

namespace Otter
{

SearchWidget::SearchWidget(QWidget *parent) : QComboBox(parent),
	m_completer(new QCompleter(this)),
	m_suggester(NULL),
	m_index(0),
	m_sendRequest(false)
{
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setCompletionMode(QCompleter::PopupCompletion);
	m_completer->setCompletionRole(Qt::DisplayRole);

	setEditable(true);
	setInsertPolicy(QComboBox::NoInsert);
	setItemDelegate(new SearchDelegate(this));
	setModel(SearchesManager::getSearchEnginesModel());
	setCurrentSearchEngine();
	optionChanged(QLatin1String("Browser/SearchEnginesSuggestions"), SettingsManager::getValue(QLatin1String("Browser/SearchEnginesSuggestions")));
	lineEdit()->setCompleter(m_completer);

	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(setCurrentSearchEngine()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(activated(int)), this, SLOT(currentSearchEngineChanged(int)));
	connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(queryChanged(QString)));
	connect(lineEdit(), SIGNAL(returnPressed()), this, SLOT(sendRequest()));
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(sendRequest(QString)));
}

void SearchWidget::hidePopup()
{
	if (!m_query.isEmpty())
	{
		m_sendRequest = true;
	}

	QComboBox::hidePopup();
}

void SearchWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Browser/SearchEnginesSuggestions"))
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

void SearchWidget::currentSearchEngineChanged(int index)
{
	if (itemData(index, Qt::AccessibleDescriptionRole).toString() == QLatin1String("configure"))
	{
		setCurrentIndex(m_index);

		PreferencesDialog dialog(QLatin1String("search"), this);
		dialog.exec();
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
	}
}

void SearchWidget::queryChanged(const QString &query)
{
	if (m_sendRequest)
	{
		sendRequest();

		m_sendRequest = false;
	}

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
	const QStringList engines = SearchesManager::getSearchEngines();

	if (engines.isEmpty())
	{
		hidePopup();
		setEnabled(false);

		lineEdit()->setPlaceholderText(QString());

		return;
	}

	setEnabled(true);

	if (sender() == SearchesManager::getInstance() && engines.contains(getCurrentSearchEngine()))
	{
		currentSearchEngineChanged(currentIndex());

		return;
	}

	const int index = qMax(0, engines.indexOf(engine.isEmpty() ? SettingsManager::getValue(QLatin1String("Browser/DefaultSearchEngine")).toString() : engine));

	setCurrentIndex(index);
	currentSearchEngineChanged(index);

	if (m_suggester)
	{
		m_suggester->setEngine(getCurrentSearchEngine());
	}
}

QString SearchWidget::getCurrentSearchEngine() const
{
	return itemData(currentIndex(), (Qt::UserRole + 1)).toString();
}

}
