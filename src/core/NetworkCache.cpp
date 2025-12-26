/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "SettingsManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>

namespace Otter
{

NetworkCache::NetworkCache(const QString &path, QObject *parent) : QNetworkDiskCache(parent)
{
	if (path.isEmpty())
	{
		return;
	}

	Utils::ensureDirectoryExists(path);

	setCacheDirectory(path);
	setMaximumCacheSize(SettingsManager::getOption(SettingsManager::Cache_DiskCacheLimitOption).toInt() * 1024);

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, [&](int identifier, const QVariant &value)
	{
		if (identifier == SettingsManager::Cache_DiskCacheLimitOption)
		{
			setMaximumCacheSize(value.toInt() * 1024);
		}
	});
}

void NetworkCache::clearCache(int period)
{
	if (period <= 0)
	{
		clear();

		emit cleared();

		return;
	}

	const QDateTime currentDateTime(QDateTime::currentDateTimeUtc());
	const QDir cacheMainDirectory(cacheDirectory());
	const QStringList directories(cacheMainDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

	for (const QString &directory: directories)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directory));
		const QStringList subDirectories(cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

		for (const QString &subDirectory: subDirectories)
		{
			const QFileInfoList files(QDir(cacheSubDirectory.absoluteFilePath(subDirectory)).entryInfoList(QDir::Files));

			for (const QFileInfo &file: files)
			{
				if (file.lastModified().toUTC().secsTo(currentDateTime) < (period * 3600))
				{
					const QNetworkCacheMetaData metaData(fileMetaData(file.absoluteFilePath()));

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

	for (const QString &directory: directories)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directory));
		const QStringList subDirectories(cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

		for (const QString &subDirectory: subDirectories)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectory));
			const QStringList files(cacheFilesDirectory.entryList(QDir::Files));

			for (const QString &file: files)
			{
				const QString cacheFilePath(cacheFilesDirectory.absoluteFilePath(file));
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

	for (const QString &directory: directories)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directory));
		const QStringList subDirectories(cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot));

		for (const QString &subDirectory: subDirectories)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectory));
			const QStringList files(cacheFilesDirectory.entryList(QDir::Files));

			entries.reserve(entries.count() + files.count());

			for (const QString &file: files)
			{
				const QNetworkCacheMetaData metaData(fileMetaData(cacheFilesDirectory.absoluteFilePath(file)));

				if (metaData.url().isValid())
				{
					entries.append(metaData.url());
				}
			}
		}
	}

	entries.squeeze();

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
