#include "SearchesManager.h"
#include "SettingsManager.h"

#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

SearchesManager* SearchesManager::m_instance = NULL;
QHash<QString, SearchInformation*> SearchesManager::m_searches;

SearchesManager::SearchesManager(QObject *parent) : QObject(parent)
{
}

void SearchesManager::createInstance(QObject *parent)
{
	m_instance = new SearchesManager(parent);
}

SearchesManager *SearchesManager::getInstance()
{
	return m_instance;
}

SearchInformation *SearchesManager::getSearch(const QString &identifier)
{
	return m_searches.value(identifier, NULL);
}

QStringList SearchesManager::getSearches()
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
		}
	}

	if (m_searches.isEmpty())
	{
		const QStringList entries = directory.entryList(QDir::Files);

		for (int i = 0; i < entries.count(); ++i)
		{
			QFile file(directory.absoluteFilePath(entries.at(i)));

			if (file.open(QIODevice::ReadOnly))
			{
				addSearch(&file, QFileInfo(entries.at(i)).baseName());

				file.close();
			}
		}
	}

	return m_searches.keys();
}

bool SearchesManager::setupQuery(const QString &query, const QString &engine, QNetworkRequest *request, QNetworkAccessManager::Operation *method, QByteArray *body)
{
	Q_UNUSED(body)

	if (!m_searches.contains(engine))
	{
		return false;
	}

	const SearchUrl search = getSearch(engine)->searchUrl;
	QString url = search.url;
	QHash<QString, QString> parameters;
	parameters["searchTerms"] = query;
	parameters["count"] = QString();
	parameters["startIndex"] = QString();
	parameters["startPage"] = QString();
	parameters["language"] = QLocale::system().name();
	parameters["inputEncoding"] = "UTF-8";
	parameters["outputEncoding"] = "UTF-8";

	QHash<QString, QString>::iterator parametersIterator;

	for (parametersIterator = parameters.begin(); parametersIterator != parameters.end(); ++parametersIterator)
	{
		url = url.replace(QString("{%1}").arg(parametersIterator.key()), parametersIterator.value());
	}

	request->setUrl(QUrl(url));
	request->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

	*method = ((search.method == "post") ? QNetworkAccessManager::PostOperation : QNetworkAccessManager::GetOperation);

	return true;
}

bool SearchesManager::addSearch(QIODevice *device, const QString &identifier)
{
	SearchInformation *search = new SearchInformation();
	search->identifier = identifier;

	SearchUrl *currentUrl = NULL;
	QXmlStreamReader reader(device->readAll());

	if (reader.readNextStartElement() && reader.name() == "OpenSearchDescription")
	{
		while (reader.readNextStartElement())
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
					currentUrl = &search->searchUrl;
				}

				if (currentUrl)
				{
					currentUrl->url = reader.attributes().value("template").toString();
					currentUrl->enctype = reader.attributes().value("enctype").toString().toLower();
					currentUrl->method = reader.attributes().value("method").toString().toLower();
				}
			}
			else if (reader.name() == "Param" && currentUrl)
			{
				currentUrl->parameters[reader.attributes().value("name").toString()] = reader.attributes().value("value").toString();
			}
			else
			{
				currentUrl = NULL;

				if (reader.name() == "ShortName")
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
				else
				{
					reader.skipCurrentElement();
				}
			}
		}
	}
	else
	{
		delete search;

		return false;
	}

	m_searches[identifier] = search;

	return true;
}

}
