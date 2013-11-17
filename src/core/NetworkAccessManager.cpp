#include "NetworkAccessManager.h"
#include "LocalListingNetworkReply.h"

#include <QtCore/QFileInfo>

namespace Otter
{

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QNetworkAccessManager(parent)
{
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
