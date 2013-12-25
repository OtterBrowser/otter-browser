#include "NetworkCache.h"
#include "SettingsManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

namespace Otter
{

NetworkCache::NetworkCache(QObject *parent) : QNetworkDiskCache(parent)
{
	setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
	setMaximumCacheSize(SettingsManager::getValue("Cache/DiskCacheLimit").toInt() * 1024);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
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
	const QStringList directories = cacheMainDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

	for (int i = 0; i < directories.count(); ++i)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directories.at(i)));
		const QStringList subDirectories = cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

		for (int j = 0; j < subDirectories.count(); ++j)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectories.at(j)));
			const QStringList files = cacheFilesDirectory.entryList(QDir::Files);

			for (int k = 0; k < files.count(); ++k)
			{
				const QNetworkCacheMetaData metaData = fileMetaData(cacheFilesDirectory.absoluteFilePath(files.at(k)));

				if (metaData.isValid() && metaData.lastModified().isValid() && metaData.lastModified().secsTo(QDateTime::currentDateTime()) > (period * 3600))
				{
					remove(metaData.url());
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
	QIODevice *device = QNetworkDiskCache::prepare(metaData);

	if (device)
	{
		m_devices[device] = metaData.url();
	}

	return device;
}

QList<QUrl> NetworkCache::getEntries() const
{
	QList<QUrl> entries;

	const QDir cacheMainDirectory(cacheDirectory());
	const QStringList directories = cacheMainDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

	for (int i = 0; i < directories.count(); ++i)
	{
		const QDir cacheSubDirectory(cacheMainDirectory.absoluteFilePath(directories.at(i)));
		const QStringList subDirectories = cacheSubDirectory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

		for (int j = 0; j < subDirectories.count(); ++j)
		{
			const QDir cacheFilesDirectory(cacheSubDirectory.absoluteFilePath(subDirectories.at(j)));
			const QStringList files = cacheFilesDirectory.entryList(QDir::Files);

			for (int k = 0; k < files.count(); ++k)
			{
				const QNetworkCacheMetaData metaData = fileMetaData(cacheFilesDirectory.absoluteFilePath(files.at(k)));

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
	const bool result = QNetworkDiskCache::remove(url);

	if (result)
	{
		emit entryRemoved(url);
	}

	return result;
}

void NetworkCache::optionChanged(const QString &option, const QVariant &value)
{
	if (option == "Cache/DiskCacheLimit")
	{
		setMaximumCacheSize(value.toInt() * 1024);
	}
}

}
