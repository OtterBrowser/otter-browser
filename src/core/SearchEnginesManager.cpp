/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SearchEnginesManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "ThemesManager.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

SearchEnginesManager* SearchEnginesManager::m_instance = NULL;
QStandardItemModel* SearchEnginesManager::m_searchEnginesModel = NULL;
QStringList SearchEnginesManager::m_searchEnginesOrder;
QStringList SearchEnginesManager::m_searchKeywords;
QHash<QString, SearchEnginesManager::SearchEngineDefinition> SearchEnginesManager::m_searchEngines;
bool SearchEnginesManager::m_isInitialized = false;

SearchEnginesManager::SearchEnginesManager(QObject *parent) : QObject(parent)
{
}

void SearchEnginesManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SearchEnginesManager(parent);
	}
}

void SearchEnginesManager::initialize()
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		loadSearchEngines();

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), m_instance, SLOT(optionChanged(int)));
	}
}

void SearchEnginesManager::optionChanged(int identifier)
{
	if (identifier == SettingsManager::Search_SearchEnginesOrderOption)
	{
		loadSearchEngines();
	}
}

void SearchEnginesManager::loadSearchEngines()
{
	m_searchEngines.clear();
	m_searchKeywords.clear();

	m_searchEnginesOrder = SettingsManager::getValue(SettingsManager::Search_SearchEnginesOrderOption).toStringList();

	const QStringList searchEnginesOrder(m_searchEnginesOrder);

	for (int i = 0; i < searchEnginesOrder.count(); ++i)
	{
		QFile file(SessionsManager::getReadableDataPath(QLatin1String("searches/") + searchEnginesOrder.at(i) + QLatin1String(".xml")));

		if (!file.open(QIODevice::ReadOnly))
		{
			m_searchEnginesOrder.removeAll(searchEnginesOrder.at(i));

			continue;
		}

		SearchEngineDefinition searchEngine(loadSearchEngine(&file, searchEnginesOrder.at(i), true));

		file.close();

		if (searchEngine.identifier.isEmpty())
		{
			m_searchEnginesOrder.removeAll(searchEnginesOrder.at(i));
		}
		else
		{
			m_searchEngines[searchEnginesOrder.at(i)] = searchEngine;
		}
	}

	emit m_instance->searchEnginesModified();

	updateSearchEnginesModel();
	updateSearchEnginesOptions();
}

void SearchEnginesManager::addSearchEngine(const SearchEngineDefinition &searchEngine, bool isDefault)
{
	if (!saveSearchEngine(searchEngine))
	{
		return;
	}

	if (isDefault)
	{
		SettingsManager::setValue(SettingsManager::Search_DefaultSearchEngineOption, searchEngine.identifier);
	}

	if (m_searchEnginesOrder.contains(searchEngine.identifier))
	{
		emit m_instance->searchEnginesModified();

		updateSearchEnginesModel();
		updateSearchEnginesOptions();
	}
	else
	{
		m_searchEnginesOrder.append(searchEngine.identifier);

		SettingsManager::setValue(SettingsManager::Search_SearchEnginesOrderOption, m_searchEnginesOrder);
	}
}

void SearchEnginesManager::updateSearchEnginesModel()
{
	if (!m_searchEnginesModel)
	{
		return;
	}

	m_searchEnginesModel->clear();

	const QStringList searchEngines(getSearchEngines());

	for (int i = 0; i < searchEngines.count(); ++i)
	{
		const SearchEngineDefinition search(getSearchEngine(searchEngines.at(i)));

		if (!search.identifier.isEmpty())
		{
			QStandardItem *item(new QStandardItem((search.icon.isNull() ? ThemesManager::getIcon(QLatin1String("edit-find")) : search.icon), QString()));
			item->setData(search.title, Qt::UserRole);
			item->setData(search.identifier, (Qt::UserRole + 1));
			item->setData(search.keyword, (Qt::UserRole + 2));
			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

			m_searchEnginesModel->appendRow(item);
		}
	}

	if (searchEngines.count() > 0)
	{
		QStandardItem *separatorItem(new QStandardItem());
		separatorItem->setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
		separatorItem->setData(QSize(-1, 10), Qt::SizeHintRole);
		separatorItem->setFlags(Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		QStandardItem *manageItem(new QStandardItem(ThemesManager::getIcon(QLatin1String("configure")), tr("Manage Search Engines…")));
		manageItem->setData(QLatin1String("configure"), Qt::AccessibleDescriptionRole);
		manageItem->setFlags(manageItem->flags() | Qt::ItemNeverHasChildren);

		m_searchEnginesModel->appendRow(separatorItem);
		m_searchEnginesModel->appendRow(manageItem);
	}

	emit m_instance->searchEnginesModelModified();
}

void SearchEnginesManager::updateSearchEnginesOptions()
{
	const QStringList searchEngines(m_searchEngines.keys());
	SettingsManager::OptionDefinition defaultQuickSearchEngineOption(SettingsManager::getOptionDefinition(SettingsManager::Search_DefaultQuickSearchEngineOption));
	defaultQuickSearchEngineOption.choices = searchEngines;

	SettingsManager::OptionDefinition defaultSearchEngineOption(SettingsManager::getOptionDefinition(SettingsManager::Search_DefaultSearchEngineOption));
	defaultSearchEngineOption.choices = searchEngines;

	SettingsManager::updateOptionDefinition(SettingsManager::Search_DefaultQuickSearchEngineOption, defaultQuickSearchEngineOption);
	SettingsManager::updateOptionDefinition(SettingsManager::Search_DefaultSearchEngineOption, defaultSearchEngineOption);
}

void SearchEnginesManager::setupQuery(const QString &query, const SearchUrl &searchUrl, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body)
{
	if (searchUrl.url.isEmpty())
	{
		return;
	}

	QString urlString(searchUrl.url);
	QHash<QString, QString> values;
	values[QLatin1String("searchTerms")] = query;
	values[QLatin1String("count")] = QString();
	values[QLatin1String("startIndex")] = QString();
	values[QLatin1String("startPage")] = QString();
	values[QLatin1String("language")] = QLocale::system().name();
	values[QLatin1String("inputEncoding")] = QLatin1String("UTF-8");
	values[QLatin1String("outputEncoding")] = QLatin1String("UTF-8");

	QHash<QString, QString>::iterator valuesIterator;

	for (valuesIterator = values.begin(); valuesIterator != values.end(); ++valuesIterator)
	{
		urlString = urlString.replace(QStringLiteral("{%1}").arg(valuesIterator.key()), QUrl::toPercentEncoding(valuesIterator.value()));
	}

	*method = ((searchUrl.method == QLatin1String("post")) ? QNetworkAccessManager::PostOperation : QNetworkAccessManager::GetOperation);

	QUrl url(urlString);
	QUrlQuery getQuery(url);
	QUrlQuery postQuery;
	const QList<QPair<QString, QString> > parameters(searchUrl.parameters.queryItems(QUrl::FullyDecoded));

	for (int i = 0; i < parameters.count(); ++i)
	{
		QString value(parameters.at(i).second);

		for (valuesIterator = values.begin(); valuesIterator != values.end(); ++valuesIterator)
		{
			value = value.replace(QStringLiteral("{%1}").arg(valuesIterator.key()), valuesIterator.value());
		}

		if (*method == QNetworkAccessManager::GetOperation)
		{
			getQuery.addQueryItem(parameters.at(i).first, QUrl::toPercentEncoding(value));
		}
		else
		{
			if (searchUrl.enctype == QLatin1String("application/x-www-form-urlencoded"))
			{
				postQuery.addQueryItem(parameters.at(i).first, QUrl::toPercentEncoding(value));
			}
			else if (searchUrl.enctype == QLatin1String("multipart/form-data"))
			{
				QString encodedValue;
				QByteArray plainValue(value.toUtf8());
				const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

				for (int j = 0; j < plainValue.length(); ++j)
				{
					const char character(plainValue[j]);

					if (character == 32 || (character >= 33 && character <= 126 && character != 61))
					{
						encodedValue.append(character);
					}
					else
					{
						encodedValue.append(QLatin1Char('='));
						encodedValue.append(hex[((character >> 4) & 0x0F)]);
						encodedValue.append(hex[(character & 0x0F)]);
					}
				}

				body->append(QLatin1String("--AaB03x\r\ncontent-disposition: form-data; name=\"") + parameters.at(i).first + QLatin1String("\"\r\ncontent-type: text/plain;charset=UTF-8\r\ncontent-transfer-encoding: quoted-printable\r\n") + encodedValue + QLatin1String("\r\n--AaB03x\r\n"));
			}
		}
	}

	if (*method == QNetworkAccessManager::PostOperation)
	{
		if (searchUrl.enctype == QLatin1String("application/x-www-form-urlencoded"))
		{
			QUrl postUrl;
			postUrl.setQuery(postQuery);

			*body = postUrl.toString().mid(1).toUtf8();
		}
		else if (searchUrl.enctype == QLatin1String("multipart/form-data"))
		{
			request->setRawHeader(QStringLiteral("Content-Type").toLatin1(), QStringLiteral("multipart/form-data; boundary=AaB03x").toLatin1());
			request->setRawHeader(QStringLiteral("Content-Length").toLatin1(), QString::number(body->length()).toUtf8());
		}
	}

	url.setQuery(getQuery);

	request->setUrl(url);
	request->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
}

SearchEnginesManager::SearchEngineDefinition SearchEnginesManager::loadSearchEngine(QIODevice *device, const QString &identifier, bool checkKeyword)
{
	SearchEngineDefinition searchEngine;
	searchEngine.identifier = identifier;

	QXmlStreamReader reader(device->readAll());

	if (reader.readNextStartElement() && reader.name() == QLatin1String("OpenSearchDescription"))
	{
		SearchUrl *currentUrl(NULL);

		while (!reader.atEnd())
		{
			reader.readNext();

			if (reader.isStartElement())
			{
				if (reader.name() == QLatin1String("Url"))
				{
					if (reader.attributes().value(QLatin1String("rel")) == QLatin1String("self") || reader.attributes().value(QLatin1String("type")) == QLatin1String("application/opensearchdescription+xml"))
					{
						searchEngine.selfUrl = QUrl(reader.attributes().value(QLatin1String("template")).toString());
					}
					else if (reader.attributes().value(QLatin1String("rel")) == QLatin1String("suggestions") || reader.attributes().value(QLatin1String("type")) == QLatin1String("application/x-suggestions+json"))
					{
						currentUrl = &searchEngine.suggestionsUrl;
					}
					else if (!reader.attributes().hasAttribute(QLatin1String("rel")) || reader.attributes().value(QLatin1String("rel")) == QLatin1String("results"))
					{
						currentUrl = &searchEngine.resultsUrl;
					}
					else
					{
						currentUrl = NULL;
					}

					if (currentUrl)
					{
						currentUrl->url = reader.attributes().value(QLatin1String("template")).toString();
						currentUrl->enctype = reader.attributes().value(QLatin1String("enctype")).toString().toLower();
						currentUrl->method = reader.attributes().value(QLatin1String("method")).toString().toLower();
					}
				}
				else if ((reader.name() == QLatin1String("Param") || reader.name() == QLatin1String("Parameter")) && currentUrl)
				{
					currentUrl->parameters.addQueryItem(reader.attributes().value(QLatin1String("name")).toString(), reader.attributes().value(QLatin1String("value")).toString());
				}
				else if (reader.name() == QLatin1String("Shortcut"))
				{
					const QString keyword(reader.readElementText());

					if (!keyword.isEmpty())
					{
						if (!m_searchKeywords.contains(keyword))
						{
							searchEngine.keyword = keyword;

							m_searchKeywords.append(keyword);
						}
						else if (!checkKeyword)
						{
							searchEngine.keyword = keyword;
						}
					}
				}
				else if (reader.name() == QLatin1String("ShortName"))
				{
					searchEngine.title = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("Description"))
				{
					searchEngine.description = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("InputEncoding"))
				{
					searchEngine.encoding = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("Image"))
				{
					const QString data(reader.readElementText());

					searchEngine.icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()), "png")));
				}
				else if (reader.name() == QLatin1String("SearchForm"))
				{
					searchEngine.formUrl = QUrl(reader.readElementText());
				}
			}
		}
	}
	else
	{
		searchEngine.identifier = QString();
	}

	return searchEngine;
}

SearchEnginesManager* SearchEnginesManager::getInstance()
{
	return m_instance;
}

QStandardItemModel* SearchEnginesManager::getSearchEnginesModel()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	if (!m_searchEnginesModel)
	{
		m_searchEnginesModel = new QStandardItemModel(m_instance);

		m_instance->updateSearchEnginesModel();
	}

	return m_searchEnginesModel;
}

SearchEnginesManager::SearchEngineDefinition SearchEnginesManager::getSearchEngine(const QString &identifier, bool byKeyword)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	if (byKeyword)
	{
		if (!identifier.isEmpty())
		{
			QHash<QString, SearchEngineDefinition>::iterator iterator;

			for (iterator = m_searchEngines.begin(); iterator != m_searchEngines.end(); ++iterator)
			{
				if (iterator.value().keyword == identifier)
				{
					return iterator.value();
				}
			}
		}

		return SearchEngineDefinition();
	}

	if (identifier.isEmpty())
	{
		return m_searchEngines.value(SettingsManager::getValue(SettingsManager::Search_DefaultSearchEngineOption).toString(), SearchEngineDefinition());
	}

	return m_searchEngines.value(identifier, SearchEngineDefinition());
}

QStringList SearchEnginesManager::getSearchEngines()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_searchEnginesOrder;
}

QStringList SearchEnginesManager::getSearchKeywords()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_searchKeywords;
}

bool SearchEnginesManager::saveSearchEngine(const SearchEngineDefinition &searchEngine)
{
	if (SessionsManager::isReadOnly() || searchEngine.identifier.isEmpty())
	{
		return false;
	}

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("searches")));

	QFile file(SessionsManager::getWritableDataPath(QLatin1String("searches/") + searchEngine.identifier + QLatin1String(".xml")));

	if (!file.open(QFile::WriteOnly))
	{
		return false;
	}

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.setAutoFormattingIndent(-1);
	writer.writeStartDocument();
	writer.writeStartElement(QLatin1String("OpenSearchDescription"));
	writer.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://a9.com/-/spec/opensearch/1.1/"));

	if (!searchEngine.formUrl.isEmpty())
	{
		writer.writeAttribute(QLatin1String("xmlns:moz"), QLatin1String("http://www.mozilla.org/2006/browser/search/"));
	}

	writer.writeTextElement(QLatin1String("Shortcut"), searchEngine.keyword);
	writer.writeTextElement(QLatin1String("ShortName"), searchEngine.title);
	writer.writeTextElement(QLatin1String("Description"), searchEngine.description);
	writer.writeTextElement(QLatin1String("InputEncoding"), searchEngine.encoding);

	if (!searchEngine.icon.isNull())
	{
		const QSize size(searchEngine.icon.availableSizes().value(0, QSize(16, 16)));
		QByteArray data;
		QBuffer buffer(&data);
		buffer.open(QIODevice::WriteOnly);

		searchEngine.icon.pixmap(size).save(&buffer, "PNG");

		writer.writeStartElement(QLatin1String("Image"));
		writer.writeAttribute(QLatin1String("width"), QString::number(size.width()));
		writer.writeAttribute(QLatin1String("height"), QString::number(size.height()));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("image/png"));
		writer.writeCharacters(QStringLiteral("data:image/png;base64,%1").arg(QString(data.toBase64())));
		writer.writeEndElement();
	}

	if (!searchEngine.selfUrl.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("self"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("application/opensearchdescription+xml"));
		writer.writeAttribute(QLatin1String("template"), searchEngine.selfUrl.toString());
		writer.writeEndElement();
	}

	if (!searchEngine.resultsUrl.url.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("results"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("text/html"));
		writer.writeAttribute(QLatin1String("method"), searchEngine.resultsUrl.method.toUpper());
		writer.writeAttribute(QLatin1String("enctype"), searchEngine.resultsUrl.enctype.toLower());
		writer.writeAttribute(QLatin1String("template"), searchEngine.resultsUrl.url);

		const QList<QPair<QString, QString> > parameters(searchEngine.resultsUrl.parameters.queryItems());

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement(QLatin1String("Param"));
			writer.writeAttribute(QLatin1String("name"), parameters.at(i).first);
			writer.writeAttribute(QLatin1String("value"), parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	if (!searchEngine.suggestionsUrl.url.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("suggestions"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("application/x-suggestions+json"));
		writer.writeAttribute(QLatin1String("method"), searchEngine.suggestionsUrl.method.toUpper());
		writer.writeAttribute(QLatin1String("enctype"), searchEngine.suggestionsUrl.enctype.toLower());
		writer.writeAttribute(QLatin1String("template"), searchEngine.suggestionsUrl.url);

		const QList<QPair<QString, QString> > parameters(searchEngine.suggestionsUrl.parameters.queryItems());

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement(QLatin1String("Param"));
			writer.writeAttribute(QLatin1String("name"), parameters.at(i).first);
			writer.writeAttribute(QLatin1String("value"), parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	if (!searchEngine.formUrl.isEmpty())
	{
		writer.writeTextElement(QLatin1String("moz:SearchForm"), searchEngine.formUrl.toString());
	}

	writer.writeEndElement();
	writer.writeEndDocument();

	return true;
}

bool SearchEnginesManager::setupSearchQuery(const QString &query, const QString &searchEngine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body)
{
	const SearchEngineDefinition search(getSearchEngine(searchEngine));

	if (search.identifier.isEmpty())
	{
		return false;
	}

	setupQuery(query, search.resultsUrl, request, method, body);

	return true;
}

}
