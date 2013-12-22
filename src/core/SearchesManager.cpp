#include "SearchesManager.h"
#include "SettingsManager.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

SearchesManager* SearchesManager::m_instance = NULL;
QStringList SearchesManager::m_order;
QStringList SearchesManager::m_shortcuts;
QHash<QString, SearchInformation*> SearchesManager::m_engines;

SearchesManager::SearchesManager(QObject *parent) : QObject(parent)
{
}

void SearchesManager::createInstance(QObject *parent)
{
	m_instance = new SearchesManager(parent);
}

SearchInformation* SearchesManager::readSearch(QIODevice *device, const QString &identifier)
{
	SearchInformation *search = new SearchInformation();
	search->identifier = identifier;

	SearchUrl *currentUrl = NULL;
	QXmlStreamReader reader(device->readAll());

	if (reader.readNextStartElement() && reader.name() == "OpenSearchDescription")
	{
		while (!reader.atEnd())
		{
			reader.readNext();

			if (reader.isStartElement())
			{
				if (reader.name() == "Url")
				{
					if (reader.attributes().value("rel") == "self")
					{
						search->selfUrl = reader.attributes().value("template").toString();
					}
					else if (reader.attributes().value("rel") == "suggestions" || reader.attributes().value("type") == "application/x-suggestions+json")
					{
						currentUrl = &search->suggestionsUrl;
					}
					else if (!reader.attributes().hasAttribute("rel") || reader.attributes().value("rel") == "results")
					{
						currentUrl = &search->resultsUrl;
					}
					else
					{
						currentUrl = NULL;
					}

					if (currentUrl)
					{
						currentUrl->url = reader.attributes().value("template").toString();
						currentUrl->enctype = reader.attributes().value("enctype").toString().toLower();
						currentUrl->method = reader.attributes().value("method").toString().toLower();
					}
				}
				else if ((reader.name() == "Param" || reader.name() == "Parameter") && currentUrl)
				{
					currentUrl->parameters.addQueryItem(reader.attributes().value("name").toString(), reader.attributes().value("value").toString());
				}
				else if (reader.name() == "Shortcut")
				{
					const QString shortcut = reader.readElementText();

					if (!shortcut.isEmpty() && !m_shortcuts.contains(shortcut))
					{
						search->shortcut = shortcut;

						m_shortcuts.append(shortcut);
					}
				}
				else if (reader.name() == "ShortName")
				{
					search->title = reader.readElementText();
				}
				else if (reader.name() == "Description")
				{
					search->description = reader.readElementText();
				}
				else if (reader.name() == "InputEncoding")
				{
					search->encoding = reader.readElementText();
				}
				else if (reader.name() == "Image")
				{
					const QString data = reader.readElementText();

					search->icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf("base64,") + 7).toUtf8()), "png")));
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

SearchInformation* SearchesManager::getEngine(const QString &identifier)
{
	return m_engines.value(identifier, NULL);
}

QStringList SearchesManager::getEngines()
{
	const QString path = SettingsManager::getPath() + "/searches/";

	QDir().mkpath(path);

	const QDir directory(path);

	if (directory.entryList(QDir::Files).isEmpty())
	{
		const QStringList definitions = QDir(":/searches/").entryList(QDir::Files);

		for (int i = 0; i < definitions.count(); ++i)
		{
			QFile::copy(":/searches/" + definitions.at(i), directory.filePath(definitions.at(i)));
			QFile::setPermissions(directory.filePath(definitions.at(i)), (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));
		}
	}

	if (m_engines.isEmpty())
	{
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
					m_engines[identifier] = search;
				}

				file.close();
			}
		}

		m_order = SettingsManager::getValue("Browser/SearchEnginesOrder").toStringList();
		m_order.removeAll(QString());

		if (m_order.isEmpty())
		{
			QStringList engines = m_engines.keys();
			engines.sort();

			m_order = engines;
		}
	}

	return m_order;
}

QStringList SearchesManager::getShortcuts()
{
	return m_shortcuts;
}

bool SearchesManager::writeSearch(QIODevice *device, SearchInformation *search)
{
	QXmlStreamWriter writer(device);
	writer.setAutoFormatting(true);
	writer.setAutoFormattingIndent(-1);
	writer.writeStartDocument();
	writer.writeStartElement("OpenSearchDescription");
	writer.writeAttribute("xmlns", "http://a9.com/-/spec/opensearch/1.1/");
	writer.writeTextElement("Shortcut", search->shortcut);
	writer.writeTextElement("ShortName", search->title);
	writer.writeTextElement("Description", search->description);
	writer.writeTextElement("InputEncoding", search->encoding);

	if (!search->icon.isNull())
	{
		const QSize size = search->icon.availableSizes().first();
		QByteArray data;
		QBuffer buffer(&data);
		buffer.open(QIODevice::WriteOnly);

		search->icon.pixmap(size).save(&buffer, "PNG");

		writer.writeStartElement("Image");
		writer.writeAttribute("width", QString::number(size.width()));
		writer.writeAttribute("height", QString::number(size.height()));
		writer.writeAttribute("type", "image/png");
		writer.writeCharacters(QString("data:image/png;base64,%1").arg(QString(data.toBase64())));
		writer.writeEndElement();
	}

	if (!search->selfUrl.isEmpty())
	{
		writer.writeStartElement("Url");
		writer.writeAttribute("type", "application/opensearchdescription+xml");
		writer.writeAttribute("template", search->selfUrl);
		writer.writeEndElement();
	}

	if (!search->resultsUrl.url.isEmpty())
	{
		writer.writeStartElement("Url");
		writer.writeAttribute("rel", "results");
		writer.writeAttribute("type", "text/html");
		writer.writeAttribute("method", search->resultsUrl.method.toUpper());
		writer.writeAttribute("enctype", search->resultsUrl.enctype.toLower());
		writer.writeAttribute("template", search->resultsUrl.url);

		const QList<QPair<QString, QString> > parameters = search->resultsUrl.parameters.queryItems();

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement("Param");
			writer.writeAttribute("name", parameters.at(i).first);
			writer.writeAttribute("value", parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	if (!search->suggestionsUrl.url.isEmpty())
	{
		writer.writeStartElement("Url");
		writer.writeAttribute("rel", "suggestions");
		writer.writeAttribute("type", "text/html");
		writer.writeAttribute("method", search->suggestionsUrl.method.toUpper());
		writer.writeAttribute("enctype", search->suggestionsUrl.enctype.toLower());
		writer.writeAttribute("template", search->suggestionsUrl.url);

		const QList<QPair<QString, QString> > parameters = search->suggestionsUrl.parameters.queryItems();

		for (int i = 0; i < parameters.count(); ++i)
		{
			writer.writeStartElement("Param");
			writer.writeAttribute("name", parameters.at(i).first);
			writer.writeAttribute("value", parameters.at(i).second);
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	writer.writeEndElement();
	writer.writeEndDocument();

	return true;
}

bool SearchesManager::setupQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body)
{
	if (!m_engines.contains(engine))
	{
		return false;
	}

	const SearchUrl search = getEngine(engine)->resultsUrl;
	QString urlString = search.url;
	QHash<QString, QString> values;
	values["searchTerms"] = query;
	values["count"] = QString();
	values["startIndex"] = QString();
	values["startPage"] = QString();
	values["language"] = QLocale::system().name();
	values["inputEncoding"] = "UTF-8";
	values["outputEncoding"] = "UTF-8";

	QHash<QString, QString>::iterator valuesIterator;

	for (valuesIterator = values.begin(); valuesIterator != values.end(); ++valuesIterator)
	{
		urlString = urlString.replace(QString("{%1}").arg(valuesIterator.key()), valuesIterator.value());
	}

	*method = ((search.method == "post") ? QNetworkAccessManager::PostOperation : QNetworkAccessManager::GetOperation);

	QUrl url(urlString);
	QUrlQuery getQuery(url);
	QUrlQuery postQuery;
	const QList<QPair<QString, QString> > parameters = search.parameters.queryItems(QUrl::FullyDecoded);

	for (int i = 0; i < parameters.count(); ++i)
	{
		QString value = parameters.at(i).second;

		for (valuesIterator = values.begin(); valuesIterator != values.end(); ++valuesIterator)
		{
			value = value.replace(QString("{%1}").arg(valuesIterator.key()), valuesIterator.value());
		}

		if (*method == QNetworkAccessManager::GetOperation)
		{
			getQuery.addQueryItem(parameters.at(i).first, QUrl::toPercentEncoding(value));
		}
		else
		{
			if (search.enctype == "application/x-www-form-urlencoded")
			{
				postQuery.addQueryItem(parameters.at(i).first, QUrl::toPercentEncoding(value));
			}
			else if (search.enctype == "multipart/form-data")
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

				body->append("--AaB03x\r\ncontent-disposition: form-data; name=\"" + parameters.at(i).first +  "\"\r\ncontent-type: text/plain;charset=UTF-8\r\ncontent-transfer-encoding: quoted-printable\r\n" + encodedValue + "\r\n--AaB03x\r\n");
			}
		}
	}

	if (*method == QNetworkAccessManager::PostOperation)
	{
		if (search.enctype == "application/x-www-form-urlencoded")
		{
			QUrl postUrl;
			postUrl.setQuery(postQuery);

			*body = postUrl.toString().mid(1).toUtf8();
		}
		else if (search.enctype == "multipart/form-data")
		{
			request->setRawHeader("Content-Type", "multipart/form-data; boundary=AaB03x");
			request->setRawHeader("Content-Length", QString::number(body->length()).toUtf8());
		}
	}

	getQuery.removeAllQueryItems("sourceid");
	getQuery.addQueryItem("sourceid", "otter");

	url.setQuery(getQuery);

	request->setUrl(url);
	request->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

	return true;
}

bool SearchesManager::setEngines(QList<SearchInformation*> engines)
{
	QList<SearchInformation*> existingEngines = m_engines.values();

	for (int i = 0; i < existingEngines.count(); ++i)
	{
		if (!engines.contains(existingEngines.at(i)))
		{
			const QString path = SettingsManager::getPath() + "/searches/" + QString(existingEngines.at(i)->identifier).remove(QRegularExpression("[\\/\\\\]")) + ".xml";

			if (QFile::exists(path) && !QFile::remove(path))
			{
				return false;
			}

			m_engines.remove(existingEngines.at(i)->identifier);
			m_order.removeAll(existingEngines.at(i)->identifier);

			delete existingEngines.at(i);

			existingEngines[i] = NULL;
		}
	}

	m_order.clear();

	for (int i = 0; i < engines.count(); ++i)
	{
		if (engines.at(i)->identifier.isEmpty())
		{
			delete engines.at(i);

			continue;
		}

		QFile file(SettingsManager::getPath() + "/searches/" + QString(engines.at(i)->identifier).remove(QRegularExpression("[\\/\\\\]")) + ".xml");

		if (!file.open(QIODevice::WriteOnly) || !writeSearch(&file, engines.at(i)))
		{
			for (int j = i; j < engines.count(); ++j)
			{
				delete engines.at(j);
			}

			SettingsManager::setValue("Browser/SearchEnginesOrder", m_order);

			emit m_instance->searchEnginesModified();

			return false;
		}

		file.close();

		m_engines[engines.at(i)->identifier] = engines.at(i);

		m_order.append(engines.at(i)->identifier);
	}

	SettingsManager::setValue("Browser/SearchEnginesOrder", m_order);

	emit m_instance->searchEnginesModified();

	return true;
}

}
