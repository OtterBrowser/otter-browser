/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "QtWebEngineUrlRequestInterceptor.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

namespace Otter
{

QtWebEngineUrlRequestInterceptor::QtWebEngineUrlRequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent),
	m_areImagesEnabled(SettingsManager::getValue(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("disabled"))
{
	QTimer::singleShot(1800000, this, SLOT(clearContentBlockingInformation()));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(handleOptionChanged(int)));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant,QUrl)), this, SLOT(handleOptionChanged(int)));
}

void QtWebEngineUrlRequestInterceptor::clearContentBlockingInformation()
{
	m_blockedElements.clear();
	m_contentBlockingProfiles.clear();

	QTimer::singleShot(1800000, this, SLOT(clearContentBlockingInformation()));
}

void QtWebEngineUrlRequestInterceptor::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::ContentBlocking_ProfilesOption:
			clearContentBlockingInformation();

			break;
		case SettingsManager::Permissions_EnableImagesOption:
			m_areImagesEnabled = (SettingsManager::getValue(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("disabled"));

			break;
		default:
			break;
	}
}

QStringList QtWebEngineUrlRequestInterceptor::getBlockedElements(const QString &domain) const
{
	return m_blockedElements.value(domain);
}

void QtWebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &request)
{
	if (!m_areImagesEnabled && request.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage)
	{
		request.block(true);

		return;
	}

	if (!m_contentBlockingProfiles.contains(request.firstPartyUrl().host()))
	{
		if (SettingsManager::getValue(SettingsManager::ContentBlocking_EnableContentBlockingOption, request.firstPartyUrl()).toBool())
		{
			m_contentBlockingProfiles[request.firstPartyUrl().host()] = ContentBlockingManager::getProfileList(SettingsManager::getValue(SettingsManager::ContentBlocking_ProfilesOption, request.firstPartyUrl()).toStringList());
		}
		else
		{
			m_contentBlockingProfiles[request.firstPartyUrl().host()] = QVector<int>();
		}
	}

	const QVector<int> contentBlockingProfiles(m_contentBlockingProfiles.value(request.firstPartyUrl().host()));

	if (contentBlockingProfiles.isEmpty())
	{
		const NetworkManagerFactory::DoNotTrackPolicy doNotTrackPolicy(NetworkManagerFactory::getDoNotTrackPolicy());

		if (doNotTrackPolicy != NetworkManagerFactory::SkipTrackPolicy)
		{
			request.setHttpHeader(QStringLiteral("DNT").toLatin1(), ((doNotTrackPolicy == NetworkManagerFactory::DoNotAllowToTrackPolicy) ? QStringLiteral("1") : QStringLiteral("0")).toLatin1());
		}

		return;
	}

	NetworkManager::ResourceType resourceType(NetworkManager::OtherType);
	bool storeBlockedUrl(true);

	switch (request.resourceType())
	{
		case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
			resourceType = NetworkManager::MainFrameType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
			resourceType = NetworkManager::SubFrameType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
			resourceType = NetworkManager::StyleSheetType;
			storeBlockedUrl = false;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeScript:
			resourceType = NetworkManager::ScriptType;
			storeBlockedUrl = false;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeImage:
			resourceType = NetworkManager::ImageType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeObject:
		case QWebEngineUrlRequestInfo::ResourceTypeMedia:
			resourceType = NetworkManager::ObjectType;

			break;
#if QT_VERSION >= 0x050700
		case QWebEngineUrlRequestInfo::ResourceTypePluginResource:
			resourceType = NetworkManager::ObjectSubrequestType;
			storeBlockedUrl = false;

			break;
#endif
		case QWebEngineUrlRequestInfo::ResourceTypeXhr:
			resourceType = NetworkManager::XmlHttpRequestType;

			break;
		default:
			break;
	}

	const ContentBlockingManager::CheckResult result(ContentBlockingManager::checkUrl(contentBlockingProfiles, request.firstPartyUrl(), request.requestUrl(), resourceType));

	if (result.isBlocked)
	{
		if (storeBlockedUrl && !m_blockedElements.value(request.firstPartyUrl().host()).contains(request.requestUrl().url()))
		{
			m_blockedElements[request.firstPartyUrl().host()].append(request.requestUrl().url());
		}

		Console::addMessage(QCoreApplication::translate("main", "Request blocked with rule: %1").arg(result.rule), Console::NetworkCategory, Console::LogLevel, request.requestUrl().toString(), -1);

		request.block(true);
	}
}

}
