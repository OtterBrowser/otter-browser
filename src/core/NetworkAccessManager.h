#ifndef OTTER_NETWORKACCESSMANAGER_H
#define OTTER_NETWORKACCESSMANAGER_H

#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

class NetworkAccessManager : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit NetworkAccessManager(QObject *parent = NULL);

};

}

#endif
