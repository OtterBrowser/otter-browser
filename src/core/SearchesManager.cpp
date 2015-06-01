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

#include "SearchesManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

SearchesManager* SearchesManager::m_instance = NULL;
QStandardItemModel* SearchesManager::m_searchEnginesModel = NULL;
QStringList SearchesManager::m_searchEnginesOrder;
QStringList SearchesManager::m_searchKeywords;
QHash<QString, SearchInformation> SearchesManager::m_searchEngines;
bool SearchesManager::m_isInitialized = false;

SearchesManager::SearchesManager(QObject *parent) : QObject(parent)
{
}

void SearchesManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SearchesManager(parent);
	}
}

void SearchesManager::initialize()
{
	if (!m_isInitialized)
	{
		m_isInitialized = true;

		loadSearchEngines();

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), m_instance, SLOT(optionChanged(QString)));
	}
}

void SearchesManager::optionChanged(const QString &key)
{
	if (key == QLatin1String("Search/SearchEnginesOrder"))
	{
		loadSearchEngines();
	}
}

void SearchesManager::loadSearchEngines()
{
	m_searchEngines.clear();
	m_searchKeywords.clear();

	m_searchEnginesOrder = SettingsManager::getValue(QLatin1String("Search/SearchEnginesOrder")).toStringList();

	const QStringList searchEnginesOrder = m_searchEnginesOrder;

	for (int i = 0; i < searchEnginesOrder.count(); ++i)
	{
		QFile file(SessionsManager::getReadableDataPath(QLatin1String("searches/") + searchEnginesOrder.at(i) + QLatin1String(".xml")));

		if (!file.open(QIODevice::ReadOnly))
		{
			m_searchEnginesOrder.removeAll(searchEnginesOrder.at(i));

			continue;
		}

		const SearchInformation engine = loadSearchEngine(&file, searchEnginesOrder.at(i));

		file.close();

		if (engine.identifier.isEmpty())
		{
			m_searchEnginesOrder.removeAll(searchEnginesOrder.at(i));
		}
		else
		{
			m_searchEngines[searchEnginesOrder.at(i)] = engine;
		}
	}

	emit m_instance->searchEnginesModified();

	updateSearchEnginesModel();
}

void SearchesManager::addSearchEngine(const SearchInformation &engine, bool isDefault)
{
	if (saveSearchEngine(engine))
	{
		if (isDefault)
		{
			SettingsManager::setValue(QLatin1String("Search/DefaultSearchEngine"), engine.identifier);
		}

		if (m_searchEnginesOrder.contains(engine.identifier))
		{
			emit m_instance->searchEnginesModified();

			updateSearchEnginesModel();
		}
		else
		{
			m_searchEnginesOrder.append(engine.identifier);

			SettingsManager::setValue(QLatin1String("Search/SearchEnginesOrder"), m_searchEnginesOrder);
		}
	}
}

void SearchesManager::updateSearchEnginesModel()
{
	if (!m_searchEnginesModel)
	{
		return;
	}

	m_searchEnginesModel->clear();

	const QStringList engines = getSearchEngines();

	for (int i = 0; i < engines.count(); ++i)
	{
		const SearchInformation search = getSearchEngine(engines.at(i));

		if (!search.identifier.isEmpty())
		{
			QStandardItem *item = new QStandardItem((search.icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : search.icon), QString());
			item->setData(search.title, Qt::UserRole);
			item->setData(search.identifier, (Qt::UserRole + 1));
			item->setData(search.keyword, (Qt::UserRole + 2));

			m_searchEnginesModel->appendRow(item);
		}
	}

	if (engines.count() > 0)
	{
		QStandardItem *separatorItem = new QStandardItem();
		separatorItem->setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
		separatorItem->setData(QSize(-1, 10), Qt::SizeHintRole);

		QStandardItem *manageItem = new QStandardItem(Utils::getIcon(QLatin1String("configure")), tr("Manage Search Engines…"));
		manageItem->setData(QLatin1String("configure"), Qt::AccessibleDescriptionRole);

		m_searchEnginesModel->appendRow(separatorItem);
		m_searchEnginesModel->appendRow(manageItem);
	}

	emit m_instance->searchEnginesModelModified();
}

void SearchesManager::setupQuery(const QString &query, const SearchUrl &searchUrl, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body)
{
	if (searchUrl.url.isEmpty())
	{
		return;
	}

	QString urlString = searchUrl.url;
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
	const QList<QPair<QString, QString> > parameters = searchUrl.parameters.queryItems(QUrl::FullyDecoded);

	for (int i = 0; i < parameters.count(); ++i)
	{
		QString value = parameters.at(i).second;

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
				QByteArray plainValue = value.toUtf8();
				const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

				for (int j = 0; j < plainValue.length(); ++j)
				{
					const char character = plainValue[j];

					if (character == 32 || (character >= 33 && character <= 126 && character != 61))
					{
						encodedValue.append(character);
					}
					else
					{
						encodedValue.append('=');
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

	getQuery.removeAllQueryItems(QLatin1String("sourceid"));
	getQuery.addQueryItem(QLatin1String("sourceid"), QLatin1String("otter"));

	url.setQuery(getQuery);

	request->setUrl(url);
	request->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
}

SearchInformation SearchesManager::loadSearchEngine(QIODevice *device, const QString &identifier)
{
	SearchInformation engine;
	engine.identifier = identifier;

	QXmlStreamReader reader(device->readAll());

	if (reader.readNextStartElement() && reader.name() == QLatin1String("OpenSearchDescription"))
	{
		SearchUrl *currentUrl = NULL;

		while (!reader.atEnd())
		{
			reader.readNext();

			if (reader.isStartElement())
			{
				if (reader.name() == QLatin1String("Url"))
				{
					if (reader.attributes().value(QLatin1String("rel")) == QLatin1String("self"))
					{
						engine.selfUrl = reader.attributes().value(QLatin1String("template")).toString();
					}
					else if (reader.attributes().value(QLatin1String("rel")) == QLatin1String("suggestions") || reader.attributes().value(QLatin1String("type")) == QLatin1String("application/x-suggestions+json"))
					{
						currentUrl = &engine.suggestionsUrl;
					}
					else if (!reader.attributes().hasAttribute(QLatin1String("rel")) || reader.attributes().value(QLatin1String("rel")) == QLatin1String("results"))
					{
						currentUrl = &engine.resultsUrl;
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
					const QString keyword = reader.readElementText();

					if (!keyword.isEmpty() && !m_searchKeywords.contains(keyword))
					{
						engine.keyword = keyword;

						m_searchKeywords.append(keyword);
					}
				}
				else if (reader.name() == QLatin1String("ShortName"))
				{
					engine.title = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("Description"))
				{
					engine.description = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("InputEncoding"))
				{
					engine.encoding = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("Image"))
				{
					const QString data = reader.readElementText();

					engine.icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()), "png")));
				}
			}
		}
	}
	else
	{
		engine.identifier = QString();
	}

	return engine;
}

SearchesManager* SearchesManager::getInstance()
{
	return m_instance;
}

QStandardItemModel* SearchesManager::getSearchEnginesModel()
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

SearchInformation SearchesManager::getSearchEngine(const QString &identifier, bool byKeyword)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	if (byKeyword)
	{
		if (!identifier.isEmpty())
		{
			QHash<QString, SearchInformation>::iterator iterator;

			for (iterator = m_searchEngines.begin(); iterator != m_searchEngines.end(); ++iterator)
			{
				if (iterator.value().keyword == identifier)
				{
					return iterator.value();
				}
			}
		}

		return SearchInformation();
	}

	if (identifier.isEmpty())
	{
		return m_searchEngines.value(SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString(), SearchInformation());
	}

	return m_searchEngines.value(identifier, SearchInformation());
}

QStringList SearchesManager::getSearchEngines()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_searchEnginesOrder;
}

QStringList SearchesManager::getSearchKeywords()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_searchKeywords;
}

bool SearchesManager::saveSearchEngine(const SearchInformation &engine)
{
	if (engine.identifier.isEmpty())
	{
		return false;
	}

	QFile file(SessionsManager::getWritableDataPath(QLatin1String("searches/") + engine.identifier + QLatin1String(".xml")));

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
	writer.writeTextElement(QLatin1String("Shortcut"), engine.keyword);
	writer.writeTextElement(QLatin1String("ShortName"), engine.title);
	writer.writeTextElement(QLatin1String("Description"), engine.description);
	writer.writeTextElement(QLatin1String("InputEncoding"), engine.encoding);

	if (!engine.icon.isNull())
	{
		const QSize size = engine.icon.availableSizes().value(0, QSize(16, 16));
		QByteArray data;
		QBuffer buffer(&data);
		buffer.open(QIODevice::WriteOnly);

		engine.icon.pixmap(size).save(&buffer, "PNG");

		writer.writeStartElement(QLatin1String("Image"));
		writer.writeAttribute(QLatin1String("width"), QString::number(size.width()));
		writer.writeAttribute(QLatin1String("height"), QString::number(size.height()));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("image/png"));
		writer.writeCharacters(QStringLiteral("data:image/png;base64,%1").arg(QString(data.toBase64())));
		writer.writeEndElement();
	}

	if (!engine.selfUrl.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("application/opensearchdescription+xml"));
		writer.writeAttribute(QLatin1String("template"), engine.selfUrl);
		writer.writeEndElement();
	}

	if (!engine.resultsUrl.url.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("results"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("text/html"));
		writer.writeAttribute(QLatin1String("method"), engine.resultsUrl.method.toUpper());
		writer.writeAttribute(QLatin1String("enctype"), engine.resultsUrl.enctype.toLower());
		writer.writeAttribute(QLatin1String("template"), engine.resultsUrl.url);

		const QList<QPair<QString, QString> > parameters = engine.resultsUrl.parameters.queryItems();

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement(QLatin1String("Param"));
			writer.writeAttribute(QLatin1String("name"), parameters.at(i).first);
			writer.writeAttribute(QLatin1String("value"), parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	if (!engine.suggestionsUrl.url.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("suggestions"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("application/x-suggestions+json"));
		writer.writeAttribute(QLatin1String("method"), engine.suggestionsUrl.method.toUpper());
		writer.writeAttribute(QLatin1String("enctype"), engine.suggestionsUrl.enctype.toLower());
		writer.writeAttribute(QLatin1String("template"), engine.suggestionsUrl.url);

		const QList<QPair<QString, QString> > parameters = engine.suggestionsUrl.parameters.queryItems();

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement(QLatin1String("Param"));
			writer.writeAttribute(QLatin1String("name"), parameters.at(i).first);
			writer.writeAttribute(QLatin1String("value"), parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	writer.writeEndElement();
	writer.writeEndDocument();

	return true;
}

bool SearchesManager::setupSearchQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body)
{
	const SearchInformation search = getSearchEngine(engine);

	if (search.identifier.isEmpty())
	{
		return false;
	}

	setupQuery(query, search.resultsUrl, request, method, body);

	return true;
}

}
