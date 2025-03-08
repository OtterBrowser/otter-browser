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

#ifndef OTTER_FEEDPARSER_H
#define OTTER_FEEDPARSER_H

#include "FeedsManager.h"

#include <QtCore/QMimeType>
#include <QtCore/QXmlStreamReader>

namespace Otter
{

class DataFetchJob;

class FeedParser : public QObject
{
	Q_OBJECT

public:
	enum ParserType
	{
		UnknownParser = 0,
		AtomParser,
		RssParser
	};

	struct FeedInformation
	{
		QString title;
		QString description;
		QUrl icon;
		QDateTime lastUpdateTime;
		QMimeType mimeType;
		QMap<QString, QString> categories;
		QVector<Feed::Entry> entries;
	};

	explicit FeedParser();

	virtual void parse(DataFetchJob *data) = 0;
	virtual FeedInformation getInformation() const = 0;
	static FeedParser* createParser(Feed *feed, DataFetchJob *data);

protected:
	static QString createIdentifier(const Feed::Entry &entry);

signals:
	void parsingFinished(bool isSuccess);
};

class AtomFeedParser final : public FeedParser
{
public:
	explicit AtomFeedParser();

	void parse(DataFetchJob *data) override;
	FeedInformation getInformation() const override;

protected:
	QDateTime readDateTime(QXmlStreamReader *reader);

private:
	FeedInformation m_information;
};

class RssFeedParser final : public FeedParser
{
public:
	explicit RssFeedParser();

	void parse(DataFetchJob *data) override;
	FeedInformation getInformation() const override;

protected:
	QDateTime readDateTime(QXmlStreamReader *reader);

private:
	FeedInformation m_information;
};

}

#endif
