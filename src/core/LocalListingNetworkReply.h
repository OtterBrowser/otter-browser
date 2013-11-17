#ifndef OTTER_LOCALLISTINGNETWORKREPLY_H
#define OTTER_LOCALLISTINGNETWORKREPLY_H

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

namespace Otter
{

class LocalListingNetworkReply : public QNetworkReply
{
public:
	LocalListingNetworkReply(QObject *parent, const QNetworkRequest &request);

	qint64 bytesAvailable() const;
	qint64 readData(char *data, qint64 maxSize);
	bool isSequential() const;

public slots:
	void abort();

private:
	QByteArray m_content;
	qint64 m_offset;
};

}

#endif
