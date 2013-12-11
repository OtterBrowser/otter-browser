#include "WebBackendsManager.h"
#include "WebBackend.h"
#include "../modules/backends/web/qtwebkit/WebBackendWebKit.h"

namespace Otter
{

WebBackendsManager *WebBackendsManager::m_instance = NULL;
QHash<QString, WebBackend*> WebBackendsManager::m_backends;

WebBackendsManager::WebBackendsManager(QObject *parent) : QObject(parent)
{
	registerBackend(new WebBackendWebKit(this), "qtwebkit");
}

void WebBackendsManager::createInstance(QObject *parent)
{
	m_instance = new WebBackendsManager(parent);
}

void WebBackendsManager::registerBackend(WebBackend *backend, const QString &name)
{
	m_backends[name] = backend;
}

QStringList WebBackendsManager::getBackends()
{
	return m_backends.keys();
}

WebBackend* WebBackendsManager::getBackend(const QString &name)
{
	if (m_backends.contains(name))
	{
		return m_backends[name];
	}

	if (name.isEmpty())
	{
		return m_backends.value(m_backends.keys().first(), NULL);
	}

	return NULL;
}

}
