#include "NetworkAccessManager.h"
#include "LocalListingNetworkReply.h"

#include <QtCore/QFileInfo>
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

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
{
	if (operation == GetOperation && request.url().isLocalFile() && QFileInfo(request.url().toLocalFile()).isDir())
	{
		return new LocalListingNetworkReply(this, request);
	}

	return QNetworkAccessManager::createRequest(operation, request, outgoingData);
}

}
