/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NetworkCache.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>

namespace Otter
{

NetworkCache::NetworkCache(QObject *parent) : QNetworkDiskCache(parent)
{
	const QString cachePath(SessionsManager::getCachePath());

	if (!cachePath.isEmpty())
	{
		QDir().mkpath(cachePath);

		setCacheDirectory(cachePath);
		setMaximumCacheSize(SettingsManager::getOption(SettingsManager::Cache_DiskCacheLimitOption).toInt() * 1024);

		connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &NetworkCache::handleOptionChanged);
	}
}

void NetworkCache::handleOptionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Cache_DiskCacheLimitOption)
	{
		setMaximumCacheSize(value.toInt() * 1024);
	}
}

void NetworkCache::clearCache(int period)
{
	if (period <= 0)
	{
		clear();

		emit cleared();

		return;
	}

	const QDir cacheMainDirectory(cacheDirectory());
	const QStringList directories(cacheMainDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

	for (int i = 0; i < directories.count(); ++i)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directories.at(i)));
		const QStringList subDirectories(cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

		for (int j = 0; j < subDirectories.count(); ++j)
		{
			const QFileInfoList files(QDir(cacheSubDirectory.absoluteFilePath(subDirectories.at(j))).entryInfoList(QDir::Files));

			for (int k = 0; k < files.count(); ++k)
			{
				if (files.at(k).lastModified().secsTo(QDateTime::currentDateTime()) < (period * 3600))
				{
					const QNetworkCacheMetaData metaData(fileMetaData(files.at(k).absoluteFilePath()));

					if (metaData.isValid())
					{
						remove(metaData.url());
					}
				}
			}
		}
	}
}

void NetworkCache::insert(QIODevice *device)
{
	QNetworkDiskCache::insert(device);

	if (m_devices.contains(device))
	{
		emit entryAdded(m_devices[device]);

		m_devices.remove(device);
	}
}

QIODevice* NetworkCache::prepare(const QNetworkCacheMetaData &metaData)
{
	QIODevice *device(QNetworkDiskCache::prepare(metaData));

	if (device)
	{
		m_devices[device] = metaData.url();
	}

	return device;
}

QString NetworkCache::getPathForUrl(const QUrl &url)
{
	if (!url.isValid() || !metaData(url).isValid())
	{
		return {};
	}

	const QDir cacheMainDirectory(cacheDirectory());
	const QStringList directories(cacheMainDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

	for (int i = 0; i < directories.count(); ++i)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directories.at(i)));
		const QStringList subDirectories(cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

		for (int j = 0; j < subDirectories.count(); ++j)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectories.at(j)));
			const QStringList files(cacheFilesDirectory.entryList(QDir::Files));

			for (int k = 0; k < files.count(); ++k)
			{
				const QString cacheFilePath(cacheFilesDirectory.absoluteFilePath(files.at(k)));
				const QNetworkCacheMetaData metaData(fileMetaData(cacheFilePath));

				if (metaData.isValid() && url == metaData.url())
				{
					return cacheFilePath;
				}
			}
		}
	}

	return {};
}

QVector<QUrl> NetworkCache::getEntries() const
{
	QVector<QUrl> entries;
	const QDir cacheMainDirectory(cacheDirectory());
	const QStringList directories(cacheMainDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

	for (int i = 0; i < directories.count(); ++i)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directories.at(i)));
		const QStringList subDirectories(cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

		for (int j = 0; j < subDirectories.count(); ++j)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectories.at(j)));
			const QStringList files(cacheFilesDirectory.entryList(QDir::Files));

			for (int k = 0; k < files.count(); ++k)
			{
				const QNetworkCacheMetaData metaData(fileMetaData(cacheFilesDirectory.absoluteFilePath(files.at(k))));

				if (metaData.url().isValid())
				{
					entries.append(metaData.url());
				}
			}
		}
	}

	return entries;
}

bool NetworkCache::remove(const QUrl &url)
{
	const bool result(QNetworkDiskCache::remove(url));

	if (result)
	{
		emit entryRemoved(url);
	}

	return result;
}

}
