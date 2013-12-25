#ifndef OTTER_COOKIEJAR_H
#define OTTER_COOKIEJAR_H

#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>

namespace Otter
{

class CookieJar : public QNetworkCookieJar
{
	Q_OBJECT
	Q_ENUMS(KeepCookiesPolicy ThirdPartyCookiesAcceptPolicy)

public:
	explicit CookieJar(QObject *parent = NULL);

	enum KeepCookiesPolicy
	{
		UntilExpireKeepCookies = 0,
		UntilExitKeepCookies = 1,
		AskIfKeepCookies = 2
	};

	enum ThirdPartyCookiesAcceptPolicy
	{
		NeverAcceptCookies = 0,
		AlwaysAcceptCookies = 1,
		AcceptExistingCookies = 2
	};

	void clearCookies(int period = 0);
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
	QList<QNetworkCookie> getCookies() const;

protected:
	void timerEvent(QTimerEvent *event);
	bool insertCookie(const QNetworkCookie &cookie);
	bool deleteCookie(const QNetworkCookie &cookie);
	bool updateCookie(const QNetworkCookie &cookie);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void save();

private:
	int m_autoSaveTimer;
	bool m_enableCookies;

signals:
	void cookieAdded(QNetworkCookie cookie);
	void cookieRemoved(QNetworkCookie cookie);
};

}

#endif
