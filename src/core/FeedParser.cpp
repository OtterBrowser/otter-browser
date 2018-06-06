/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "FeedParser.h"
#include "Console.h"
#include "FeedsManager.h"
#include "Job.h"

#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>

namespace Otter
{

FeedParser::FeedParser(Feed *parent) : QObject(parent)
{
}

FeedParser* FeedParser::createParser(Feed *feed, DataFetchJob *data)
{
	const QMimeDatabase mimeDatabase;
	const QMap<QString, ParserType> parsers({{QLatin1String("application/atom+xml"), AtomParser}, {QLatin1String("application/rss+xml"), RssParser}});
	QMimeType mimeType(mimeDatabase.mimeTypeForData(data->getData()));

	if (!mimeType.isValid() || !parsers.contains(mimeType.name()))
	{
		mimeType = mimeDatabase.mimeTypeForUrl(feed->getUrl());
	}

	if ((!mimeType.isValid() || !parsers.contains(mimeType.name())) && data->getHeaders().contains(QByteArrayLiteral("Content-Type")))
	{
		QMap<QString, ParserType>::const_iterator iterator;
		const QString header(data->getHeaders().value(QByteArrayLiteral("Content-Type")));

		for (iterator = parsers.begin(); iterator != parsers.end(); ++iterator)
		{
			if (header.contains(iterator.key(), Qt::CaseInsensitive))
			{
				mimeType = mimeDatabase.mimeTypeForName(iterator.key());

				break;
			}
		}
	}

	if (parsers.contains(mimeType.name()))
	{
		switch (parsers.value(mimeType.name()))
		{
			case AtomParser:
				return new AtomFeedParser(feed);
			case RssParser:
				return new RssFeedParser(feed);
			default:
				break;
		}
	}

	return nullptr;
}

AtomFeedParser::AtomFeedParser(Feed *parent) : FeedParser(parent),
	m_feed(parent)
{
	m_feed->setMimeType(QMimeDatabase().mimeTypeForName(QLatin1String("application/atom+xml")));
}

bool AtomFeedParser::parse(DataFetchJob *data)
{
	QXmlStreamReader reader(data->getData());
	QMap<QString, QString> categories;
	QVector<Feed::Entry> entries;
	entries.reserve(10);

	if (reader.readNextStartElement() && reader.name() == QLatin1String("feed"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("entry"))
			{
				Feed::Entry entry;

				while (reader.readNext())
				{
					if (reader.isStartElement())
					{
						if (reader.name() == QLatin1String("category"))
						{
							entry.categories.append(reader.attributes().value(QLatin1String("term")).toString());

							reader.skipCurrentElement();
						}
						else if (reader.name() == QLatin1String("title"))
						{
							entry.title = reader.readElementText().simplified();
						}
						else if (reader.name() == QLatin1String("link") && reader.attributes().value(QLatin1String("rel")).toString() == QLatin1String("alternate"))
						{
							entry.url = QUrl(reader.attributes().value(QLatin1String("href")).toString());

							reader.skipCurrentElement();
						}
						else if (reader.name() == QLatin1String("id"))
						{
							entry.identifier = reader.readElementText();
						}
						else if (reader.name() == QLatin1String("published"))
						{
							entry.publicationTime = readDateTime(&reader);
						}
						else if (reader.name() == QLatin1String("updated"))
						{
							entry.updateTime = readDateTime(&reader);
						}
						else if (reader.name() == QLatin1String("summary"))
						{
							entry.summary = reader.readElementText();
						}
						else if (reader.name() == QLatin1String("content"))
						{
							entry.content = reader.readElementText(QXmlStreamReader::IncludeChildElements);
						}
						else if (reader.name() == QLatin1String("name"))
						{
							entry.author = reader.readElementText().simplified();
						}
						else if (reader.name() == QLatin1String("email"))
						{
							entry.email = reader.readElementText().simplified();
						}
						else if (reader.name() != QLatin1String("author"))
						{
							reader.skipCurrentElement();
						}
					}
					else if (reader.isEndElement() && reader.name() == QLatin1String("entry"))
					{
						break;
					}
				}

				entries.append(entry);
			}
			else if (reader.isStartElement())
			{
				if (reader.name() == QLatin1String("category"))
				{
					categories[reader.attributes().value(QLatin1String("term")).toString()] = reader.attributes().value(QLatin1String("label")).toString();

					reader.skipCurrentElement();
				}
				else if (reader.name() == QLatin1String("icon"))
				{
					m_feed->setIcon(QUrl(reader.readElementText()));
				}
				else if (reader.name() == QLatin1String("title"))
				{
					m_feed->setTitle(reader.readElementText().simplified());
				}
				else if (reader.name() == QLatin1String("summary"))
				{
					m_feed->setDescription(reader.readElementText());
				}
				else if (reader.name() == QLatin1String("updated"))
				{
					m_feed->setLastUpdateTime(readDateTime(&reader));
				}
				else
				{
					reader.skipCurrentElement();
				}
			}
			else
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError())
			{
				Console::addMessage(tr("Failed to parse feed file: %1").arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, data->getUrl().toDisplayString());
			}
		}
	}

	entries.squeeze();

	m_feed->setCategories(categories);
	m_feed->addEntries(entries);
	m_feed->setLastSynchronizationTime(QDateTime::currentDateTimeUtc());

	return true;
}

QDateTime AtomFeedParser::readDateTime(QXmlStreamReader *reader)
{
	QDateTime dateTime(QDateTime::fromString(reader->readElementText(), Qt::ISODate));
	dateTime.setTimeSpec(Qt::UTC);

	return dateTime;
}

RssFeedParser::RssFeedParser(Feed *parent) : FeedParser(parent),
	m_feed(parent)
{
	m_feed->setMimeType(QMimeDatabase().mimeTypeForName(QLatin1String("application/rss+xml")));
}

QDateTime RssFeedParser::readDateTime(QXmlStreamReader *reader)
{
	QDateTime dateTime(QDateTime::fromString(reader->readElementText(), Qt::RFC2822Date));
	dateTime.setTimeSpec(Qt::UTC);

	return dateTime;
}

bool RssFeedParser::parse(DataFetchJob *data)
{
	QXmlStreamReader reader(data->getData());
	QMap<QString, QString> categories;
	QVector<Feed::Entry> entries;
	entries.reserve(10);

	if (reader.readNextStartElement() && reader.name() == QLatin1String("rss"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("item"))
			{
				Feed::Entry entry;

				while (reader.readNext())
				{
					if (reader.isStartElement())
					{
						if (reader.name() == QLatin1String("category"))
						{
							const QString category(reader.readElementText());

							entry.categories.append(category);

							if (!categories.contains(category))
							{
								categories[category] = QString();
							}
						}
						else if (reader.name() == QLatin1String("title"))
						{
							entry.title = reader.readElementText().simplified();
						}
						else if (reader.name() == QLatin1String("link"))
						{
							entry.url = QUrl(reader.readElementText());
						}
						else if (reader.name() == QLatin1String("guid"))
						{
							const bool isLink(reader.attributes().value(QLatin1String("isPermaLink")).toString().toLower() == QLatin1String("true"));

							entry.identifier = reader.readElementText();

							if (isLink)
							{
								entry.url = QUrl(entry.identifier);
							}
						}
						else if (reader.name() == QLatin1String("pubDate"))
						{
							entry.publicationTime = readDateTime(&reader);
						}
						else if (reader.name() == QLatin1String("description"))
						{
							entry.summary = reader.readElementText(QXmlStreamReader::IncludeChildElements);
						}
						else if (reader.name() == QLatin1String("author"))
						{
							const QString text(reader.readElementText().simplified());

							if (QRegularExpression(QLatin1String("^[a-zA-Z0-9\\._\\-]+@[a-zA-Z0-9\\._\\-]+\\.[a-zA-Z0-9]+$")).match(text).hasMatch())
							{
								entry.email = text;
							}
							else
							{
								entry.author = text;
							}
						}
					}
					else if (reader.isEndElement() && reader.name() == QLatin1String("item"))
					{
						break;
					}
				}

				entries.append(entry);
			}
			else if (reader.isStartElement())
			{
				if (reader.name() == QLatin1String("image"))
				{
					while (reader.readNext())
					{
						if (reader.isStartElement() && reader.name() == QLatin1String("url"))
						{
							m_feed->setIcon(QUrl(reader.readElementText()));
						}
						else if (reader.isEndElement() && reader.name() == QLatin1String("image"))
						{
							break;
						}
					}
				}
				else if (reader.name() == QLatin1String("title"))
				{
					m_feed->setTitle(reader.readElementText().simplified());
				}
				else if (reader.name() == QLatin1String("description"))
				{
					m_feed->setDescription(reader.readElementText());
				}
				else if (reader.name() == QLatin1String("lastBuildDate"))
				{
					m_feed->setLastUpdateTime(readDateTime(&reader));
				}
				else if (reader.name() != QLatin1String("channel"))
				{
					reader.skipCurrentElement();
				}
			}
			else
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError())
			{
				Console::addMessage(tr("Failed to parse feed file: %1").arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, data->getUrl().toDisplayString(), reader.lineNumber());
			}
		}
	}

	entries.squeeze();

	m_feed->setCategories(categories);
	m_feed->addEntries(entries);
	m_feed->setLastSynchronizationTime(QDateTime::currentDateTimeUtc());

	return true;
}

}
