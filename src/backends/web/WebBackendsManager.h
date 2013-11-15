#ifndef OTTER_WEBBACKENDSMANAGER_H
#define OTTER_WEBBACKENDSMANAGER_H

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace Otter
{

class WebBackend;

class WebBackendsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void registerBackend(WebBackend *backend, const QString &name);
	static WebBackend* getBackend(const QString &backend = QString());
	static QStringList getBackends();

protected:
	explicit WebBackendsManager(QObject *parent = NULL);

private:
	static WebBackendsManager *m_instance;
	static QHash<QString, WebBackend*> m_backends;
};

}

#endif
