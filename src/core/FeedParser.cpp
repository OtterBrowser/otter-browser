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
#include "FeedsManager.h"
#include "Job.h"

namespace Otter
{

FeedParser::FeedParser(Feed *parent) : QObject(parent)
{
}

FeedParser* FeedParser::createParser(Feed *feed, DataFetchJob *data)
{
	Q_UNUSED(data)

	return new AtomFeedParser(feed);
}

AtomFeedParser::AtomFeedParser(Feed *parent) : FeedParser(parent),
	m_feed(parent)
{
}

bool AtomFeedParser::parse(DataFetchJob *data)
{
	Q_UNUSED(data)

//TODO

	return false;
}

}
