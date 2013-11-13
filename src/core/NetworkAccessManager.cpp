#include "NetworkAccessManager.h"

#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkDiskCache>

namespace Otter
{

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QNetworkAccessManager(parent)
{
	QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
	diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

	setCache(diskCache);
}

}
