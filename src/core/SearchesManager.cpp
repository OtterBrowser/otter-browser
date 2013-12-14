#include "SearchesManager.h"
#include "SettingsManager.h"

#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

namespace Otter
{

SearchesManager* SearchesManager::m_instance = NULL;
QList<SearchInformation*> SearchesManager::m_searches;

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

QStringList SearchesManager::getSearches()
{
	if (m_searches.isEmpty())
	{
		const QDir directory(SettingsManager::getPath() + "/searches/");
		const QStringList entries = directory.entryList();

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

	QStringList searches;

	for (int i = 0; i < m_searches.count(); ++i)
	{
		searches.append(m_searches.at(i)->identifier);
	}

	return searches;
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
				if (!reader.attributes().hasAttribute("rel") || reader.attributes().value("rel") == "results")
				{
					currentUrl = &search->searchUrl;
				}
				else if (reader.attributes().value("rel") == "suggestions")
				{
					currentUrl = &search->suggestionsUrl;
				}

				if (currentUrl)
				{
					currentUrl->url = reader.attributes().value("template").toString();
					currentUrl->enctype = reader.attributes().value("enctype").toString();
					currentUrl->method = reader.attributes().value("method").toString();
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

					search->icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf("base64," + 7)).toUtf8()))));
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

	m_searches.append(search);

	return true;
}

}
