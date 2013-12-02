#ifndef OTTER_COOKIEJAR_H
#define OTTER_COOKIEJAR_H

#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>

namespace Otter
{

class CookieJar : public QNetworkCookieJar
{
	Q_OBJECT

public:
	explicit CookieJar(QObject *parent = NULL);

protected:
	void timerEvent(QTimerEvent *event);
	bool deleteCookie(const QNetworkCookie &cookie);
	bool insertCookie(const QNetworkCookie &cookie);
	bool updateCookie(const QNetworkCookie &cookie);

protected slots:
	void save();

private:
	int m_autoSaveTimer;
};

}

#endif
