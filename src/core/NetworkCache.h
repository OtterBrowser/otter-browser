#ifndef OTTER_NETWORKCACHE_H
#define OTTER_NETWORKCACHE_H

#include <QtNetwork/QNetworkDiskCache>

namespace Otter
{

class NetworkCache : public QNetworkDiskCache
{
	Q_OBJECT

public:
	explicit NetworkCache(QObject *parent = NULL);

	void clearCache(int period = 0);
	void insert(QIODevice *device);
	QIODevice* prepare(const QNetworkCacheMetaData &metaData);
	bool remove(const QUrl &url);

private:
	QHash<QIODevice*, QUrl> m_devices;

signals:
	void urlInserted(QUrl url);
	void urlRemoved(QUrl url);
};

}

#endif
