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

#ifndef OTTER_FEEDPARSER_H
#define OTTER_FEEDPARSER_H

#include <QtCore/QXmlStreamReader>

namespace Otter
{

class DataFetchJob;
class Feed;

class FeedParser : public QObject
{
public:
	enum ParserType
	{
		UnknownParser = 0,
		AtomParser,
		RssParser
	};

	explicit FeedParser(Feed *parent = nullptr);

	virtual bool parse(DataFetchJob *data) = 0;
	static FeedParser* createParser(Feed *feed, DataFetchJob *data);
};

class AtomFeedParser final : public FeedParser
{
public:
	explicit AtomFeedParser(Feed *parent = nullptr);

	bool parse(DataFetchJob *data) override;

protected:
	QDateTime readDateTime(QXmlStreamReader *reader);

private:
	Feed *m_feed;
};

class RssFeedParser final : public FeedParser
{
public:
	explicit RssFeedParser(Feed *parent = nullptr);

	bool parse(DataFetchJob *data) override;

protected:
	QDateTime readDateTime(QXmlStreamReader *reader);

private:
	Feed *m_feed;
};

}

#endif
