/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
QHash<QString, SearchInformation*> SearchesManager::m_searchEngines;
bool SearchesManager::m_initialized = false;

SearchesManager::SearchesManager(QObject *parent) : QObject(parent)
{
}

SearchesManager::~SearchesManager()
{
	QHash<QString, SearchInformation*>::iterator iterator;

	for (iterator = m_searchEngines.begin(); iterator != m_searchEngines.end(); ++iterator)
	{
		delete iterator.value();
	}
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
	m_initialized = true;

	const QString path = SessionsManager::getProfilePath() + QLatin1String("/searches/");
	const QDir directory(path);

	if (!QFile::exists(path))
	{
		QDir().mkpath(path);

		if (directory.entryList(QDir::Files).isEmpty())
		{
			const QStringList definitions = QDir(QLatin1String(":/searches/")).entryList(QDir::Files);

			for (int i = 0; i < definitions.count(); ++i)
			{
				QFile::copy(QLatin1String(":/searches/") + definitions.at(i), directory.filePath(definitions.at(i)));
				QFile::setPermissions(directory.filePath(definitions.at(i)), (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));
			}
		}
	}

	const QStringList entries = directory.entryList(QDir::Files);

	for (int i = 0; i < entries.count(); ++i)
	{
		QFile file(directory.absoluteFilePath(entries.at(i)));

		if (file.open(QIODevice::ReadOnly))
		{
			const QString identifier = QFileInfo(entries.at(i)).baseName();
			SearchInformation *search = readSearch(&file, identifier);

			if (search)
			{
				m_searchEngines[identifier] = search;
			}

			file.close();
		}
	}

	m_searchEnginesOrder = SettingsManager::getValue(QLatin1String("Search/SearchEnginesOrder")).toStringList();
	m_searchEnginesOrder.removeAll(QString());

	if (m_searchEnginesOrder.isEmpty())
	{
		QStringList engines = m_searchEngines.keys();
		engines.sort();

		m_searchEnginesOrder = engines;
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
		SearchInformation *search = getSearchEngine(engines.at(i));

		if (search)
		{
			QStandardItem *item = new QStandardItem((search->icon.isNull() ? Utils::getIcon(QLatin1String("edit-find")) : search->icon), QString());
			item->setData(search->title, Qt::UserRole);
			item->setData(search->identifier, (Qt::UserRole + 1));
			item->setData(search->keyword, (Qt::UserRole + 2));
			item->setData(QSize(-1, 22), Qt::SizeHintRole);

			m_searchEnginesModel->appendRow(item);
		}
	}

	if (engines.count() > 0)
	{
		QStandardItem *separatorItem = new QStandardItem();
		separatorItem->setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
		separatorItem->setData(QSize(-1, 10), Qt::SizeHintRole);

		QStandardItem *manageItem = new QStandardItem(Utils::getIcon(QLatin1String("configure")), tr("Manage Search Engines..."));
		manageItem->setData(QLatin1String("configure"), Qt::AccessibleDescriptionRole);
		manageItem->setData(QSize(-1, 22), Qt::SizeHintRole);

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
			request->setRawHeader("Content-Type", "multipart/form-data; boundary=AaB03x");
			request->setRawHeader("Content-Length", QString::number(body->length()).toUtf8());
		}
	}

	getQuery.removeAllQueryItems(QLatin1String("sourceid"));
	getQuery.addQueryItem(QLatin1String("sourceid"), QLatin1String("otter"));

	url.setQuery(getQuery);

	request->setUrl(url);
	request->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
}

SearchInformation* SearchesManager::readSearch(QIODevice *device, const QString &identifier)
{
	SearchInformation *search = new SearchInformation();
	search->identifier = identifier;

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
						search->selfUrl = reader.attributes().value(QLatin1String("template")).toString();
					}
					else if (reader.attributes().value(QLatin1String("rel")) == QLatin1String("suggestions") || reader.attributes().value(QLatin1String("type")) == QLatin1String("application/x-suggestions+json"))
					{
						currentUrl = &search->suggestionsUrl;
					}
					else if (!reader.attributes().hasAttribute(QLatin1String("rel")) || reader.attributes().value(QLatin1String("rel")) == QLatin1String("results"))
					{
						currentUrl = &search->resultsUrl;
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
						search->keyword = keyword;

						m_searchKeywords.append(keyword);
					}
				}
				else if (reader.name() == QLatin1String("ShortName"))
				{
					search->title = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("Description"))
				{
					search->description = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("InputEncoding"))
				{
					search->encoding = reader.readElementText();
				}
				else if (reader.name() == QLatin1String("Image"))
				{
					const QString data = reader.readElementText();

					search->icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()), "png")));
				}
			}
		}
	}
	else
	{
		delete search;

		return NULL;
	}

	return search;
}

SearchesManager* SearchesManager::getInstance()
{
	return m_instance;
}

SearchInformation* SearchesManager::getSearchEngine(const QString &identifier, bool byKeyword)
{
	if (!m_initialized && m_instance)
	{
		m_instance->initialize();
	}

	if (byKeyword)
	{
		if (!identifier.isEmpty())
		{
			QHash<QString, SearchInformation*>::iterator iterator;

			for (iterator = m_searchEngines.begin(); iterator != m_searchEngines.end(); ++iterator)
			{
				if (iterator.value()->keyword == identifier)
				{
					return iterator.value();
				}
			}
		}

		return NULL;
	}

	if (identifier.isEmpty())
	{
		return m_searchEngines.value(SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString(), NULL);
	}

	return m_searchEngines.value(identifier, NULL);
}

QStandardItemModel* SearchesManager::getSearchEnginesModel()
{
	if (!m_searchEnginesModel)
	{
		m_searchEnginesModel = new QStandardItemModel(m_instance);

		m_instance->updateSearchEnginesModel();
	}

	return m_searchEnginesModel;
}

QStringList SearchesManager::getSearchEngines()
{
	if (!m_initialized && m_instance)
	{
		m_instance->initialize();
	}

	return m_searchEnginesOrder;
}

QStringList SearchesManager::getSearchKeywords()
{
	if (!m_initialized && m_instance)
	{
		m_instance->initialize();
	}

	return m_searchKeywords;
}

bool SearchesManager::writeSearch(QIODevice *device, SearchInformation *search)
{
	QXmlStreamWriter writer(device);
	writer.setAutoFormatting(true);
	writer.setAutoFormattingIndent(-1);
	writer.writeStartDocument();
	writer.writeStartElement(QLatin1String("OpenSearchDescription"));
	writer.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://a9.com/-/spec/opensearch/1.1/"));
	writer.writeTextElement(QLatin1String("Shortcut"), search->keyword);
	writer.writeTextElement(QLatin1String("ShortName"), search->title);
	writer.writeTextElement(QLatin1String("Description"), search->description);
	writer.writeTextElement(QLatin1String("InputEncoding"), search->encoding);

	if (!search->icon.isNull())
	{
		const QSize size = search->icon.availableSizes().value(0, QSize(16, 16));
		QByteArray data;
		QBuffer buffer(&data);
		buffer.open(QIODevice::WriteOnly);

		search->icon.pixmap(size).save(&buffer, "PNG");

		writer.writeStartElement(QLatin1String("Image"));
		writer.writeAttribute(QLatin1String("width"), QString::number(size.width()));
		writer.writeAttribute(QLatin1String("height"), QString::number(size.height()));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("image/png"));
		writer.writeCharacters(QStringLiteral("data:image/png;base64,%1").arg(QString(data.toBase64())));
		writer.writeEndElement();
	}

	if (!search->selfUrl.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("application/opensearchdescription+xml"));
		writer.writeAttribute(QLatin1String("template"), search->selfUrl);
		writer.writeEndElement();
	}

	if (!search->resultsUrl.url.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("results"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("text/html"));
		writer.writeAttribute(QLatin1String("method"), search->resultsUrl.method.toUpper());
		writer.writeAttribute(QLatin1String("enctype"), search->resultsUrl.enctype.toLower());
		writer.writeAttribute(QLatin1String("template"), search->resultsUrl.url);

		const QList<QPair<QString, QString> > parameters = search->resultsUrl.parameters.queryItems();

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement(QLatin1String("Param"));
			writer.writeAttribute(QLatin1String("name"), parameters.at(i).first);
			writer.writeAttribute(QLatin1String("value"), parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	if (!search->suggestionsUrl.url.isEmpty())
	{
		writer.writeStartElement(QLatin1String("Url"));
		writer.writeAttribute(QLatin1String("rel"), QLatin1String("suggestions"));
		writer.writeAttribute(QLatin1String("type"), QLatin1String("application/x-suggestions+json"));
		writer.writeAttribute(QLatin1String("method"), search->suggestionsUrl.method.toUpper());
		writer.writeAttribute(QLatin1String("enctype"), search->suggestionsUrl.enctype.toLower());
		writer.writeAttribute(QLatin1String("template"), search->suggestionsUrl.url);

		const QList<QPair<QString, QString> > parameters = search->suggestionsUrl.parameters.queryItems();

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
	SearchInformation *information = getSearchEngine(engine);

	if (information)
	{
		setupQuery(query, information->resultsUrl, request, method, body);

		return true;
	}

	return false;
}

bool SearchesManager::setSearchEngines(const QList<SearchInformation*> &engines)
{
	const QList<SearchInformation*> existingEngines = m_searchEngines.values();

	for (int i = 0; i < existingEngines.count(); ++i)
	{
		if (!engines.contains(existingEngines.at(i)))
		{
			const QString path = SessionsManager::getProfilePath() + QLatin1String("/searches/") + QString(existingEngines.at(i)->identifier).remove(QRegularExpression(QLatin1String("[\\/\\\\]"))) + QLatin1String(".xml");

			if (QFile::exists(path) && !QFile::remove(path))
			{
				emit m_instance->searchEnginesModified();

				m_instance->updateSearchEnginesModel();

				return false;
			}

			m_searchEngines.remove(existingEngines.at(i)->identifier);
			m_searchEnginesOrder.removeAll(existingEngines.at(i)->identifier);

			delete existingEngines.at(i);
		}
	}

	m_searchEnginesOrder.clear();

	for (int i = 0; i < engines.count(); ++i)
	{
		if (engines.at(i)->identifier.isEmpty())
		{
			delete engines.at(i);

			continue;
		}

		QFile file(SessionsManager::getProfilePath() + QLatin1String("/searches/") + QString(engines.at(i)->identifier).remove(QRegularExpression("[\\/\\\\]")) + ".xml");

		if (!file.open(QIODevice::WriteOnly) || !writeSearch(&file, engines.at(i)))
		{
			for (int j = i; j < engines.count(); ++j)
			{
				delete engines.at(j);
			}

			SettingsManager::setValue(QLatin1String("Search/SearchEnginesOrder"), m_searchEnginesOrder);

			emit m_instance->searchEnginesModified();

			m_instance->updateSearchEnginesModel();

			return false;
		}

		file.close();

		m_searchEngines[engines.at(i)->identifier] = engines.at(i);

		m_searchEnginesOrder.append(engines.at(i)->identifier);
	}

	SettingsManager::setValue(QLatin1String("Search/SearchEnginesOrder"), m_searchEnginesOrder);

	emit m_instance->searchEnginesModified();

	m_instance->updateSearchEnginesModel();

	return true;
}

}
