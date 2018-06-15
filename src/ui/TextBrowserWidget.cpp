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

#include "TextBrowserWidget.h"
#include "../core/Job.h"
#include "../core/NetworkCache.h"
#include "../core/NetworkManagerFactory.h"

namespace Otter
{

TextBrowserWidget::TextBrowserWidget(QWidget *parent) : QTextBrowser(parent),
	m_imagesPolicy(AllImages)
{
}

void TextBrowserWidget::setImagesPolicy(TextBrowserWidget::ImagesPolicy policy)
{
	m_imagesPolicy = policy;

	if (document())
	{
		document()->adjustSize();
	}
}

QVariant TextBrowserWidget::loadResource(int type, const QUrl &url)
{
	if ((type == QTextDocument::ImageResource && m_imagesPolicy == NoImages) || (type != QTextDocument::ImageResource && QTextDocument::StyleSheetResource))
	{
		return {};
	}

	const QVariant result(QTextBrowser::loadResource(type, url));

	if (!result.isNull() || m_resources.contains(url))
	{
		return result;
	}

	if (type == QTextDocument::ImageResource && m_imagesPolicy == OnlyCachedImages)
	{
		NetworkCache *cache(NetworkManagerFactory::getCache());

		if (cache)
		{
			QIODevice *device(cache->data(url));

			if (device)
			{
				const QByteArray resourceData(device->readAll());

				device->deleteLater();

				return resourceData;
			}
		}

		return {};
	}

	m_resources.insert(url);

	DataFetchJob *job(new DataFetchJob(url, this));

	connect(job, &DataFetchJob::jobFinished, [=](bool isSuccess)
	{
		if (isSuccess)
		{
			document()->addResource(type, url, job->getData()->readAll());
			document()->adjustSize();
		}
	});

	job->start();

	return {};
}

}
