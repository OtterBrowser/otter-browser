/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QCryptographicHash>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimeZone>

namespace Otter
{

FeedParser::FeedParser() = default;

FeedParser* FeedParser::createParser(Feed *feed, DataFetchJob *data)
{
	if (!feed || !data)
	{
		return nullptr;
	}

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
		const QString header(QString::fromLatin1(data->getHeaders().value(QByteArrayLiteral("Content-Type"))));

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
				return new AtomFeedParser();
			case RssParser:
				return new RssFeedParser();
			default:
				break;
		}
	}

	return nullptr;
}

QString FeedParser::createIdentifier(const Feed::Entry &entry)
{
	if (entry.publicationTime.isValid())
	{
		return QString::number(entry.publicationTime.toMSecsSinceEpoch());
	}

	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(entry.summary.toUtf8());
	hash.addData(entry.content.toUtf8());

	return QString::fromLatin1(hash.result());
}

AtomFeedParser::AtomFeedParser() : FeedParser()
{
	m_information.mimeType = QMimeDatabase().mimeTypeForName(QLatin1String("application/atom+xml"));
}

void AtomFeedParser::parse(DataFetchJob *data)
{
	QXmlStreamReader reader(data->getData());
	bool isSuccess(true);

	m_information.entries.reserve(10);

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
						const QXmlStreamAttributes attributes(reader.attributes());

						if (reader.name() == QLatin1String("category"))
						{
							entry.categories.append(attributes.value(QLatin1String("term")).toString());

							reader.skipCurrentElement();
						}
						else if (reader.name() == QLatin1String("title"))
						{
							entry.title = reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
						}
						else if (reader.name() == QLatin1String("link") && attributes.value(QLatin1String("rel")).toString() == QLatin1String("alternate"))
						{
							entry.url = QUrl(attributes.value(QLatin1String("href")).toString());

							reader.skipCurrentElement();
						}
						else if (reader.name() == QLatin1String("id"))
						{
							entry.identifier = reader.readElementText(QXmlStreamReader::IncludeChildElements);
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
							entry.summary = reader.readElementText(QXmlStreamReader::IncludeChildElements);
						}
						else if (reader.name() == QLatin1String("content"))
						{
							entry.content = reader.readElementText(QXmlStreamReader::IncludeChildElements);
						}
						else if (reader.name() == QLatin1String("name"))
						{
							entry.author = reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
						}
						else if (reader.name() == QLatin1String("email"))
						{
							entry.email = reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
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

				if (entry.identifier.isEmpty())
				{
					entry.identifier = createIdentifier(entry);
				}

				m_information.entries.append(entry);
			}
			else if (reader.isStartElement())
			{
				if (reader.name() == QLatin1String("category"))
				{
					const QXmlStreamAttributes attributes(reader.attributes());

					m_information.categories[attributes.value(QLatin1String("term")).toString()] = attributes.value(QLatin1String("label")).toString();

					reader.skipCurrentElement();
				}
				else if (reader.name() == QLatin1String("icon"))
				{
					m_information.icon = QUrl(reader.readElementText(QXmlStreamReader::IncludeChildElements));
				}
				else if (reader.name() == QLatin1String("title"))
				{
					m_information.title = reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
				}
				else if (reader.name() == QLatin1String("summary"))
				{
					m_information.description = reader.readElementText(QXmlStreamReader::IncludeChildElements);
				}
				else if (reader.name() == QLatin1String("updated"))
				{
					m_information.lastUpdateTime = readDateTime(&reader);
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

				isSuccess = false;
			}
		}
	}

	m_information.entries.squeeze();

	if (m_information.entries.isEmpty())
	{
		Console::addMessage(tr("Failed to parse feed: no valid entries found"), Console::NetworkCategory, Console::ErrorLevel, data->getUrl().toDisplayString());

		isSuccess = false;
	}

	emit parsingFinished(isSuccess);
}

FeedParser::FeedInformation AtomFeedParser::getInformation() const
{
	return m_information;
}

QDateTime AtomFeedParser::readDateTime(QXmlStreamReader *reader)
{
	QDateTime dateTime(QDateTime::fromString(reader->readElementText(QXmlStreamReader::IncludeChildElements), Qt::ISODate));
	dateTime.setTimeZone(QTimeZone::utc());

	return dateTime;
}

RssFeedParser::RssFeedParser() : FeedParser()
{
	m_information.mimeType = QMimeDatabase().mimeTypeForName(QLatin1String("application/rss+xml"));
}

void RssFeedParser::parse(DataFetchJob *data)
{
	QXmlStreamReader reader(data->getData());
	bool isSuccess(true);
	const QRegularExpression emailExpression(QLatin1String(R"(^[a-zA-Z0-9\._\-]+@[a-zA-Z0-9\._\-]+\.[a-zA-Z0-9]+$)"));
	emailExpression.optimize();

	m_information.entries.reserve(10);

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
							const QString category(reader.readElementText(QXmlStreamReader::IncludeChildElements));

							entry.categories.append(category);

							if (!m_information.categories.contains(category))
							{
								m_information.categories[category] = QString();
							}
						}
						else if (reader.name() == QLatin1String("title"))
						{
							entry.title = reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
						}
						else if (reader.name() == QLatin1String("link"))
						{
							entry.url = QUrl(reader.readElementText(QXmlStreamReader::IncludeChildElements));
						}
						else if (reader.name() == QLatin1String("guid"))
						{
							const bool isLink(reader.attributes().value(QLatin1String("isPermaLink")).toString().toLower() == QLatin1String("true"));

							entry.identifier = reader.readElementText(QXmlStreamReader::IncludeChildElements);

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
							const QString text(reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified());

							if (emailExpression.match(text).hasMatch())
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

				if (entry.identifier.isEmpty())
				{
					entry.identifier = createIdentifier(entry);
				}

				m_information.entries.append(entry);
			}
			else if (reader.isStartElement())
			{
				if (reader.name() == QLatin1String("image"))
				{
					while (reader.readNext())
					{
						if (reader.isStartElement() && reader.name() == QLatin1String("url"))
						{
							m_information.icon = QUrl(reader.readElementText(QXmlStreamReader::IncludeChildElements));
						}
						else if (reader.isEndElement() && reader.name() == QLatin1String("image"))
						{
							break;
						}
					}
				}
				else if (reader.name() == QLatin1String("title"))
				{
					m_information.title = reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
				}
				else if (reader.name() == QLatin1String("description"))
				{
					m_information.description = reader.readElementText(QXmlStreamReader::IncludeChildElements);
				}
				else if (reader.name() == QLatin1String("lastBuildDate"))
				{
					m_information.lastUpdateTime = readDateTime(&reader);
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
				Console::addMessage(tr("Failed to parse feed file: %1").arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, data->getUrl().toDisplayString(), static_cast<int>(reader.lineNumber()));

				isSuccess = false;
			}
		}
	}

	m_information.entries.squeeze();

	if (m_information.entries.isEmpty())
	{
		Console::addMessage(tr("Failed to parse feed: no valid entries found"), Console::NetworkCategory, Console::ErrorLevel, data->getUrl().toDisplayString());

		isSuccess = false;
	}

	emit parsingFinished(isSuccess);
}

FeedParser::FeedInformation RssFeedParser::getInformation() const
{
	return m_information;
}

QDateTime RssFeedParser::readDateTime(QXmlStreamReader *reader)
{
	QDateTime dateTime(QDateTime::fromString(reader->readElementText(QXmlStreamReader::IncludeChildElements), Qt::RFC2822Date));
	dateTime.setTimeZone(QTimeZone::utc());

	return dateTime;
}

}
