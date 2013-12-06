#include "BookmarksManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

namespace Otter
{

BookmarksManager* BookmarksManager::m_instance = NULL;
QList<Bookmark*> BookmarksManager::m_bookmarks;
QHash<int, Bookmark*> BookmarksManager::m_pointers;
QSet<QString> BookmarksManager::m_urls;
int BookmarksManager::m_identifier;

BookmarksManager::BookmarksManager(QObject *parent) : QObject(parent)
{
	QTimer::singleShot(250, this, SLOT(load()));
}

void BookmarksManager::load()
{
	m_bookmarks.clear();
	m_identifier = 0;

	QFile file(SettingsManager::getPath() + "/bookmarks.xbel");

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		return;
	}

	QXmlStreamReader reader(file.readAll());

	if (reader.readNextStartElement() && reader.name() == "xbel" && reader.attributes().value("version").toString() == "1.0")
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == "folder" || reader.name() == "bookmark" || reader.name() == "separator")
			{
				m_bookmarks.append(readBookmark(&reader));
			}
			else
			{
				reader.skipCurrentElement();
			}
		}
	}
}

void BookmarksManager::writeBookmark(QXmlStreamWriter *writer, Bookmark *bookmark)
{
	switch (bookmark->type)
	{
		case FolderBookmark:
			writer->writeStartElement("folder");
			writer->writeTextElement("title", bookmark->title);

			for (int i = 0; i < bookmark->children.count(); ++i)
			{
				writeBookmark(writer, bookmark->children.at(i));
			}

			writer->writeEndElement();

			break;
		case UrlBookmark:
			writer->writeStartElement("bookmark");

			if (!bookmark->url.isEmpty())
			{
				writer->writeAttribute("href", bookmark->url);
			}

			writer->writeTextElement("title", bookmark->title);

			if (!bookmark->description.isEmpty())
			{
				writer->writeAttribute("desc", bookmark->description);
			}

			writer->writeEndElement();

			break;
		default:
			writer->writeEmptyElement("separator");

			break;
	}
}

void BookmarksManager::createInstance(QObject *parent)
{
	m_instance = new BookmarksManager(parent);
}

BookmarksManager *BookmarksManager::getInstance()
{
	return m_instance;
}

Bookmark *BookmarksManager::readBookmark(QXmlStreamReader *reader, int parent)
{
	Bookmark *bookmark = new Bookmark();

	if (reader->name() == "folder")
	{
		bookmark->type = FolderBookmark;
		bookmark->identifier = ++m_identifier;
		bookmark->parent = parent;

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == "title")
				{
					bookmark->title = reader->readElementText().trimmed();
				}
				else if (reader->name() == "desc")
				{
					bookmark->description = reader->readElementText().trimmed();
				}
				else if (reader->name() == "folder" || reader->name() == "bookmark" || reader->name() == "separator")
				{
					bookmark->children.append(readBookmark(reader, bookmark->parent));
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == "folder")
			{
				break;
			}
		}

		m_pointers[bookmark->identifier] = bookmark;
	}
	else if (reader->name() == "bookmark")
	{
		bookmark->type = UrlBookmark;
		bookmark->url = reader->attributes().value("href").toString();

		const QUrl url(bookmark->url);

		if (url.isValid())
		{
			m_urls.insert(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
		}

		while (reader->readNext())
		{
			if (reader->isStartElement())
			{
				if (reader->name() == "title")
				{
					bookmark->title = reader->readElementText().trimmed();
				}
				else if (reader->name() == "desc")
				{
					bookmark->description = reader->readElementText().trimmed();
				}
				else
				{
					reader->skipCurrentElement();
				}
			}
			else if (reader->isEndElement() && reader->name() == "bookmark")
			{
				break;
			}
		}
	}
	else if (reader->name() == "separator")
	{
		bookmark->type = SeparatorBookmark;

		reader->readNext();
	}

	return bookmark;
}

QList<Bookmark*> BookmarksManager::getBookmarks()
{
	return m_bookmarks;
}

QList<Bookmark*> BookmarksManager::getFolder(int folder)
{
	if (folder == 0)
	{
		return m_bookmarks;
	}

	if (m_pointers.contains(folder))
	{
		return m_pointers[folder]->children;
	}

	return QList<Bookmark*>();
}

bool BookmarksManager::addBookmark(Bookmark *bookmark, int folder, int index)
{
	if (!bookmark || (folder != 0 && !m_pointers.contains(folder)))
	{
		return false;
	}

	const QUrl url(bookmark->url);

	if (url.isValid())
	{
		m_urls.insert(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
	}

	if (bookmark->type == FolderBookmark)
	{
		bookmark->identifier = ++m_identifier;

		m_pointers[bookmark->identifier] = bookmark;
	}

	bookmark->parent = folder;

	if (folder == 0)
	{
		m_bookmarks.insert(((index < 0) ? m_bookmarks.count() : index), bookmark);
	}
	else
	{
		m_pointers[folder]->children.insert(((index < 0) ? m_pointers[folder]->children.count() : index), bookmark);
	}

	emit m_instance->folderModified(folder);

	return save();
}

bool BookmarksManager::hasBookmark(const QString &url)
{
	if (url.isEmpty())
	{
		return false;
	}

	return hasBookmark(QUrl(url));
}

bool BookmarksManager::hasBookmark(const QUrl &url)
{
	if (!url.isValid())
	{
		return false;
	}

	return m_urls.contains(url.toString(QUrl::RemovePassword | QUrl::RemoveFragment));
}

bool BookmarksManager::save(const QString &path)
{
	QFile file(path.isEmpty() ? SettingsManager::getPath() + "/bookmarks.xbel" : path);

	if (!file.open(QFile::WriteOnly))
	{
		return false;
	}

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.setAutoFormattingIndent(-1);
	writer.writeStartDocument();
	writer.writeDTD(QLatin1String("<!DOCTYPE xbel>"));
	writer.writeStartElement(QLatin1String("xbel"));
	writer.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));

	for (int i = 0; i < m_bookmarks.count(); ++i)
	{
		writeBookmark(&writer, m_bookmarks.at(i));
	}

	writer.writeEndDocument();

	return true;
}

}
